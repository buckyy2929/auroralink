# M2-B — libEGD Protocol & API Usage

**Status:** Complete  
**Sources:** `libEGD-master/src/egd/client/egd_client.hpp/.cpp`, `json_client.hpp/.cpp`, `util/egd_spec.h`, `EgdTest.h`, `EgdSend.c`, `libEGD-master/README.md`

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

At the data rates involved (EGD is typically 10–100 Hz for control data, not 8 kHz), JSON overhead is negligible. The EGD side is not the latency-critical path — the Aurora side is. `EgdClient` raw bytes only makes sense if profiling shows the JSON overhead is a problem, which is unlikely given the payload sizes (≤1400 bytes).

The bridge's canonical buffer (keyed by field name) maps cleanly to JSON key-value pairs, making `JsonClient` the natural fit without any additional translation layer.

---

---

## 9. Open Design Item — EGD Config XML Fetch

**The bridge application must fetch the EGD config XML at startup.**

libEGD's `JsonClient` requires a populated `EgdPageInfo` (with `var_map`) before it can publish or subscribe. That `var_map` is built by parsing the XML config fetched from a ControlST server:

```
GET http://<host>:7937/EGD?Action=GetDoc&Type=ProducedData&Active=True&ProducerID=<id>
```

**Design requirement:** The bridge startup sequence must include an HTTP GET to the EGD config endpoint before any publish or subscribe calls are made. The bridge must not assume a static/hardcoded field layout — the config XML is the authoritative source of field names, types, and byte offsets.

**Implications for bridge design (feed into M2-D):**

| Question | Decision needed |
|---|---|
| Config server address | Must be configurable (host, port) — not hardcoded |
| Retry on failure | Config fetch must retry or fail-closed at startup — partial config is worse than no config |
| Startup ordering | Config fetch → build `var_map` → then start Aurora adapter and EGD TX/RX threads |
| ProducerID | Derived from `inet_addr(producer_host)` — must know the producer's IP at config time |
| Both directions | May need to fetch both `ProducedData` (what we consume) and `ConsumedData` (what we produce) depending on EMS server config |

**Status:** Design decision deferred to M2-D. Config server address and retry policy to be specified there.

---

*Sources: `libEGD-master/src/egd/client/egd_client.cpp` (Publish, Subscribe, PollSocket); `json_client.cpp` (JsonClient::Publish, JsonProxyCallback); `egd_spec.h` (MAX_EGD_PAYLOAD=1400, DATA_PORT=18246); `EgdTest.h` (EGD_MAX_DATA_SIZE=1400, EGD_DEFAULT_PORT=18246, Data_Production_Hdr); `EgdSend.c` (timing model, InitHeader, SendDataMessage); `libEGD-master/README.md` (config fetch URL)*
