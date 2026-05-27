# M2-B — libEGD Protocol & API Usage

**Status:** Protocol, wire format, and API characterised against the upstream libEGD source. **One explicit open design item handed off to M2-D** — EGD config XML fetch at bridge startup (§9). No on-target runtime exercise of `libEGD.so` has been performed yet (the M2-C ARM64 cross-build was verified at the link / load level; an EGD round-trip on the Kria PS is gated on board access).  
**Sources:** `libEGD-master/src/egd/client/egd_client.hpp/.cpp`, `json_client.hpp/.cpp`, `util/egd_spec.h`, `EgdTest.h`, `EgdSend.c`, `libEGD-master/README.md`

**Scope note (relative to `MILESTONE_2_TASKS.md` M2-B spec).** The spec calls for a ≤ 3-page findings document. The protocol summary and API cheat-sheet (§§1–8) are that deliverable; §9 is an explicit handoff to M2-D so the bridge implementer has the open question in front of them rather than buried.

---

## 1. EGD Protocol Background

EGD (Ethernet Global Data) is a GE industrial protocol for cyclic data exchange over UDP. It uses a **producer/consumer** model — a producer periodically broadcasts a page of data; any number of consumers receive it.

**Wire format** — every message is a fixed header followed by a raw binary payload:

```
[ DataProductionHdr (28 bytes) ][ Data payload (up to 1400 bytes) ]
```

Header fields (source: `egd_spec.h`, `EgdTest.h`):

| Field | Type | Value / Notes |
|---|---|---|
| `PDUType` | uint8 | Always **13** (Data Production) |
| `Version` | uint8 | Always **1** |
| `ReqID` | uint16 | Increments per send (sequence number) |
| `ProducerID` | uint32 | `inet_addr(producer_ip)` — IP as integer |
| `ExchangeID` | uint32 | Identifies the data page |
| `Timestamp` | 8 bytes | `tv_sec` + `tv_nsec` (POSIX timespec) |
| `Status` | uint16 | 2 = healthy/not-synced, 1 = invalid |
| `Signature` | uint16 | Config checksum — used to detect config mismatch |
| `ConfigTimeSecs` | uint32 | Time config was generated |

**Transport:** UDP, port **18246** for data, port **7937** for HTTP config fetch.  
**Endianness:** Payload fields are **little-endian** (source: `htole32`/`le32toh` in `json_client.cpp`).

---

## 2. Configuration

Before sending or receiving, libEGD fetches an XML config from a ControlST (or compatible) server:

```
GET http://<host>:7937/EGD?Action=GetDoc&Type=ProducedData&Active=True&ProducerID=<id>
```

The XML describes each exchange (page) and its variables:

```xml
<Exchange ExchangeId="1" DataLength="36" Page="DG_OUT" PeriodNSecs="10000000">
    <Var Name="VAR_NAME" DType="REAL" VOffs="192" />
</Exchange>
```

`VOffs` is a **bit offset** into the payload. `DType` maps to the types the library serializes: `BOOL`, `REAL` (float32), `LREAL` (float64), `SINT/INT/DINT/LINT`, `BYTE/USINT/WORD/UINT/DWORD/UDINT/ULINT`.

This config drives everything — JsonClient uses it to know where each named field lives in the binary payload.

---

## 3. Payload Size Limits

| Limit | Value | Source |
|---|---|---|
| Max payload (data only) | **1400 bytes** | `MAX_EGD_PAYLOAD` in `egd_spec.h`; `EGD_MAX_DATA_SIZE` in `EgdTest.h` |
| Header size | 28 bytes | `sizeof(DataProductionHdr)` in `egd_spec.h` |
| Max total UDP datagram | 1428 bytes | Header + payload |
| Socket receive buffer | 128 KB | `BUF_SIZE = 128*1024` in `egd_client.cpp` |

Payloads larger than 1400 bytes must be split across multiple exchanges (pages). `EgdSend.c` uses 50 bytes of data as its working example.

---

## 4. API Cheat Sheet

| Function | Direction | Sync model | Notes |
|---|---|---|---|
| `EgdClient::Publish()` | TX | **Synchronous** | Single `sendto()` call; returns immediately after send |
| `EgdClient::Subscribe()` | RX | **Async** | Spawns a background `std::thread` per `(host, port)` that polls the socket; callback fired from that thread |
| `EgdClient::SubscribeBlocking()` | RX | **Synchronous** | Calls `PollSocket()` on the calling thread; blocks forever |
| `JsonClient::Publish()` | TX | **Synchronous** | Parses JSON string → serializes fields to binary → calls `EgdClient::Publish()` |
| `JsonClient::Subscribe()` | RX | **Async** | Installs proxy callback that deserializes binary → JSON before calling user callback; otherwise same as `EgdClient::Subscribe()` |
| `JsonClient::SubscribeBlocking()` | RX | **Synchronous** | Same as `EgdClient::SubscribeBlocking()` but with JSON deserialization |
| `egd_json_client_init()` | — | — | C API: allocates a `JsonClient` |
| `egd_json_client_publish()` | TX | Synchronous | C API wrapper for `JsonClient::Publish()` |
| `egd_json_client_subscribe()` | RX | Async | C API wrapper for `JsonClient::Subscribe()` |
| `egd_json_client_free()` | — | — | C API: deletes the client, joins subscriber threads |

---

## 5. Publish Call Flow (TX path)

Numbered steps from application call to socket I/O:

```
1. Application calls JsonClient::Publish(host, port, page_info, json_string)
      |
2. json::parse(json_string) → JSON object
      |
3. For each key in JSON object:
      a. Look up variable in page_info.var_map (from config XML)
      b. Determine byte_offset = var.bit_offset / 8
      c. Serialize value into DataProductionMessage.Data[byte_offset]
         - Applies htole32/htole64 for multi-byte types (little-endian)
         - BOOL: bit-shifted into correct bit position
      |
4. Calls EgdClient::Publish(host, port, page_info, data, 1400)
      |
5. First call for this (host, port, producer_id, exchange_id):
      a. socket(AF_INET, SOCK_DGRAM, 0)
      b. setsockopt SO_BROADCAST
      c. Caches EgdPubInfo in publisher_map_
      |
6. Builds DataProductionHdr:
      - PDUType=13, Version=1
      - ProducerID, ExchangeID from page_info
      - Timestamp from clock_gettime(CLOCK_REALTIME)
      - Status, Signature from page_info
      - ReqID increments each call
      |
7. sendto(socket, &message, sizeof(hdr)+data_len, 0, &addr, sizeof(addr))
      |
8. Returns EgdStatus::OK
```

---

## 6. Subscribe Call Flow (RX path)

```
1. Application calls JsonClient::Subscribe(host, port, page_info, user_callback, user_data)
      |
2. Stores JsonUserData{user_callback, user_data} in json_user_data_ vector
      |
3. Calls EgdClient::Subscribe(host, port, page_info, JsonProxyCallback, &json_user_data)
      |
4. First call for this (host, port):
      a. socket(AF_INET, SOCK_DGRAM, 0)
      b. setsockopt SO_REUSEADDR
      c. bind(socket, addr)  ← binds to port 18246 on host IP
      d. setsockopt SO_RCVBUF = 128KB
      e. Spawns std::thread → PollSocket()
      f. Caches EgdSubInfo in subscriber_map_
      |
5. Adds {JsonProxyCallback, page_info, json_user_data} to sub_info.callbacks[]
      |
── [Background thread running PollSocket()] ──────────────────────────
      |
6. poll(socket, POLL_TIMEOUT_MS=1000ms) — blocks until data or timeout
      |
7. recvfrom(socket) → DataProductionMessage buffer
      |
8. Validate message:
      - PDUType == 13
      - Version == 1
      - ProducerID matches page_info
      - ExchangeID matches page_info
      - Status field → EGD_MESSAGE_VALID / EGD_MESSAGE_UNHEALTHY
      - Signature matches → EGD_MESSAGE_SIG_ERROR if not
      |
9. For each registered callback:
      JsonProxyCallback(page_info, header, status, data, data_len, user_data)
         |
10.    For each var in page_info.var_map:
            byte_offset = var.bit_offset / 8
            Deserialize data[byte_offset] → typed value
            Apply le32toh/le64toh (little-endian)
            Add to json_message[var_name]
         |
11.    Call user_callback(page_info, header, status, json_string, user_data)
```

---

## 7. Sync vs Async Summary

| Scenario | Mechanism | Thread model |
|---|---|---|
| **Publish (TX)** | Synchronous `sendto()` | Caller's thread; no internal thread created |
| **Subscribe (RX) — default** | `poll()` loop in background thread | 1 thread per unique `(host, port)` pair; callbacks fire from that thread |
| **Subscribe (RX) — blocking** | `poll()` loop on calling thread | Blocks caller forever; only use for simple single-exchange consumers |
| **Destructor cleanup** | `run_flag_ = false`, then `thread::join()` | Clean shutdown; subscriber threads exit on next poll timeout (≤1 s) |

**Key implication for the bridge:** The subscriber callback is fired from libEGD's internal thread. The bridge must not do heavy work in the callback — write to the shared buffer and return immediately.

---

## 8. Recommendation: JsonClient vs EgdClient

**Use `JsonClient` for this bridge.**

| Factor | JsonClient | EgdClient |
|---|---|---|
| Field encoding | Automatic (from config XML) | Manual — you write bit packing |
| Field naming | JSON keys = variable names | Byte offsets only |
| Config alignment with M2-A | Aligns with configurable field-map approach | Requires separate mapping layer |
| Endianness | Handled internally | Caller's responsibility |
| Performance cost | JSON parse/serialize per message | None |
| Maintainability | High | Low — brittle byte offsets |

At the data rates plausibly involved in this bridge, JSON overhead is negligible. For context, two rates are in play and **both are unsourced in this repo today**:

- The **Aurora-side** rate (the "125 µs / 8 kHz" number) is the M2-A frame rate; it is set by the RTDS model, not by Aurora itself. It is **not** sourced from an RTDS docs citation in this repo — see M2-A Gap 10.2.
- The **EGD-side** rate is governed by the `<Exchange PeriodNSecs="…">` value in the config XML; ControlST EGD pages commonly run at 10 ms – 100 ms periods (= 10–100 Hz) for cyclic control data, but the **authoritative rate for this deployment will be whatever the config XML says** (§2 above; §9 below). The "10–100 Hz" range is an order-of-magnitude expectation, not a measured fact for this customer.

The architectural recommendation does not depend on the exact numbers: as long as the Aurora rate is materially higher than the EGD rate (the typical setup), the bridge's EGD-side latency budget per message is large enough that JSON parse/serialise overhead is well below the per-message budget. If the rates ever converge — for example a future EGD page at 1 kHz against an 8 kHz Aurora frame — then `EgdClient` raw bytes can be revisited by profiling; for the current scope `JsonClient` is the right default. Payload size also helps: at ≤ 1400 bytes per message, even worst-case JSON costs are well inside typical CPU budgets.

The bridge's canonical buffer (keyed by field name) maps cleanly to JSON key-value pairs, making `JsonClient` the natural fit without any additional translation layer.

---

## 9. M2-D Handoff — Open Design Item: EGD Config XML Fetch

> **This section is an explicit handoff to M2-D, not an M2-B deliverable.** It is recorded here because the answer to "how does the bridge get its EGD field map?" is an M2-B-level fact about libEGD; the decisions about *when*, *how*, and *from where* belong to the bridge architecture (M2-D, see `docs/BRIDGE_SOFTWARE_DESIGN.md`).

**The bridge application must fetch the EGD config XML at startup before any libEGD publish or subscribe call.**

libEGD's `JsonClient` requires a populated `EgdPageInfo` (with `var_map`) before it can publish or subscribe. That `var_map` is built by parsing the XML config fetched from a ControlST (or compatible) server:

```
GET http://<host>:7937/EGD?Action=GetDoc&Type=ProducedData&Active=True&ProducerID=<id>
```

The bridge MUST NOT assume a static / hard-coded field layout — the config XML is the authoritative source of field names, types, and bit offsets (the `VOffs` attribute described in §2).

**Design questions that M2-D must answer (none of these are answered in M2-B):**

| # | Question | Why M2-D, not M2-B |
|---|---|---|
| 9.1 | Config server address — hard-coded, CLI arg, env var, or service-discovered? | This is a deployment / runtime config decision, owned by the bridge architecture. |
| 9.2 | Retry / back-off policy on fetch failure. Fail-closed at startup vs serve stale cached config vs retry-forever? | Tied to bridge operational behaviour — what the bridge should do when EGD config server is briefly unreachable. |
| 9.3 | Startup ordering between Aurora-adapter init, EGD config fetch, and EGD TX/RX threads | A bridge-thread-graph question, not a libEGD question. |
| 9.4 | `ProducerID` derivation — `inet_addr(producer_host)`; which IP at multi-NIC time? | A bridge-deployment question. |
| 9.5 | Whether the bridge needs to fetch both `Type=ProducedData` (consumed-by-bridge) and `Type=ConsumedData` (produced-by-bridge); both directions, or only one, depending on EMS server config | Depends on the bridge's role at this customer, which is specified in M2-D. |
| 9.6 | Config-XML caching on the Kria filesystem (so the bridge can boot if the EMS server is briefly down) — yes/no, and where? | Bridge persistence policy. |

**M2-B's contribution to these questions:** the protocol-level facts (URL, port 7937, what the XML must contain to satisfy `JsonClient`) are answered above. The architecture decisions are M2-D's.

**Status:** all six sub-items above are explicit open items handed off to M2-D. M2-B is otherwise complete.

---

*Sources: `libEGD-master/src/egd/client/egd_client.cpp` (Publish, Subscribe, PollSocket); `json_client.cpp` (JsonClient::Publish, JsonProxyCallback); `egd_spec.h` (MAX_EGD_PAYLOAD=1400, DATA_PORT=18246); `EgdTest.h` (EGD_MAX_DATA_SIZE=1400, EGD_DEFAULT_PORT=18246, Data_Production_Hdr); `EgdSend.c` (timing model, InitHeader, SendDataMessage); `libEGD-master/README.md` (config fetch URL)*
