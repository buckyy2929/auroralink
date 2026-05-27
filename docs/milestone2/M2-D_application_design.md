# M2-D — Bridge Application Architecture (Bidirectional Dataflow)

**Status:** Architecture designed; implementation is M3 scope. Interface contract (`bridge/include/aurora_adapter.h`), field-map schema (`config/aurora_fieldmap.example.yaml`), and stub (`bridge/src/aurora_adapter.c`) are committed and compile-clean. Every function in the stub returns `AURORA_ERR_NOT_IMPLEMENTED` — no hardware I/O, no thread, no YAML loader in the committed code today.

**Sources:** `DESIGN_INTENT.md`, `BRIDGE_SOFTWARE_DESIGN.md`, `M2-A_auroralink_findings.md` (FIFO register map, IRQ wiring, UIO re-arm), `M2-B_libegd_findings.md` (EGD API, config XML), `M2-C_cross_compile.md` (ARM64 library at `out/arm64/`), `bridge/include/aurora_adapter.h`, `config/aurora_fieldmap.example.yaml`, `RTDS_Aurora_Link/.../Axi_IO.c`, `Axi_IO.h`.

**Honest framing.** The reviewer has the right to push on three things up front:
1. **Does any of this run on the board?** No. `aurora_adapter.c` is a stub. The board verification plan is in M2-A §11 (stages S1–S3). M2-D produces the architecture; M3 produces the implementation.
2. **Is the 8 kHz Aurora frame rate confirmed?** No. The rate is set by the RTDS model. It is unsourced in this repo (M2-A Gap 10.2). The architecture is rate-agnostic; the design works for any Aurora-side inter-frame interval.
3. **Is Gap 10.8 (refclk mismatch) resolved?** Resolution is proposed and parametrised in the TCL script; it is not yet Vivado-validated (M2-A §12.7). M2-D's architecture is independent of whether the Aurora IP runs at 2 Gbps / 125 MHz or 2.5 Gbps / 156.25 MHz — the software path is identical.

---

## 1. Architecture Overview

The bridge is a single Linux process on the Kria A53 running Ubuntu. It owns four active threads plus the EGD RX thread managed internally by libEGD.

```
══════════════════════════════════════════════════════════════════════════════
  HARDWARE / KERNEL                BRIDGE PROCESS (Ubuntu A53)
══════════════════════════════════════════════════════════════════════════════

 RTDS (remote)                 ┌─────────────────────────────────────────────┐
    │                          │  T1: Aurora RX Worker                       │
    │ fiber                    │  poll(/dev/uio0)                            │
    ▼                          │  → drain FIFO (37 words)                   │
 [Aurora IP, PL]               │  → decode via field-map                    │
    │ AXI-Stream                │  → write to aurora_to_egd_buf (mutex)      │
    ▼                          │  → re-arm UIO                              │
 [AXI FIFO, PL]  ──RC IRQ──►  │  → signal T2 (condvar)                     │
    │                          └──────────────────┬──────────────────────────┘
    │ mmap /dev/uio0                              │
    │ (0xA0010000)             ┌──────────────────▼──────────────────────────┐
    │                          │  Canonical Buffer A → EGD                   │
    │                          │  aurora_to_egd_buf {                        │
    │                          │    field[0..36]: {name, value, wire_type}   │
    │                          │    state: EMPTY|VALID|STALE|INVALID         │
    │                          │    last_rx_ns: CLOCK_MONOTONIC timestamp    │
    │                          │  }   (mutex protected, simple struct)       │
    │                          └──────────────────┬──────────────────────────┘
    │                                             │
    │                          ┌──────────────────▼──────────────────────────┐
    │                          │  T2: EGD TX Publisher                       │
    │                          │  wakes on condvar OR periodic timer         │
    │                          │  → check state (VALID / STALE / INVALID)   │
    │                          │  → construct JSON from field values         │
    │                          │  → JsonClient::Publish() → UDP sendto()    │
    │                          └─────────────────────────────────────────────┘
    │                                                │
    │                                               UDP 18246 → EGD network
    │
    │                          ┌─────────────────────────────────────────────┐
    │                          │  T3 (libEGD internal): EGD RX Thread        │
    │                          │  JsonClient::Subscribe() background thread  │
    │                          │  → validate EGD message                    │
    │                          │  → decode JSON fields                      │
    │                          │  → on_egd_rx_callback():                   │
    │                          │      write to egd_to_aurora_buf (mutex)    │
    │                          │      signal T4 (condvar)                   │
    │                          └─────────────────────────────────────────────┘
    │                                        │
    │                          ┌─────────────▼───────────────────────────────┐
    │                          │  Canonical Buffer EGD → Aurora              │
    │                          │  egd_to_aurora_buf {                        │
    │                          │    field[0..22]: {name, value}              │
    │                          │    state: EMPTY|VALID|STALE|INVALID         │
    │                          │    last_rx_ns: CLOCK_MONOTONIC timestamp    │
    │                          │  }   (mutex protected)                      │
    │                          └─────────────▬───────────────────────────────┘
    │                                        │
    │                          ┌─────────────▼───────────────────────────────┐
    │                          │  T4: Aurora TX Worker                       │
    │                          │  wakes on condvar (new EGD RX data)         │
    │                          │  → pack aurora_tx_field_t[] from buffer     │
    │                          │  → aurora_adapter_send_tx()                 │
    │                          │      write 23 words to TX FIFO              │
    │                          │      set TLR register                       │
    │                          └──────────────────────────────────────────────┘
    │
    │                          ┌─────────────────────────────────────────────┐
    │                          │  T5: Supervisor / Health Thread             │
    │                          │  wakes every ~100 ms (configurable)         │
    │                          │  → check aurora_to_egd_buf.last_rx_ns      │
    │                          │  → if elapsed > stale_threshold_ms → STALE │
    │                          │  → log aurora_adapter_get_stats()          │
    │                          │  → write health file (optional)            │
    │                          └─────────────────────────────────────────────┘
    │
    └────── [AXI FIFO, PL] ◄── TX FIFO write (T4: aurora_adapter_send_tx)
               │ AXI-Stream
               ▼
          [Aurora IP, PL]
               │ fiber
               ▼
           RTDS (remote)

══════════════════════════════════════════════════════════════════════════════
  EGD network (UDP 18246)        ◄──── T2 sends; T3 receives (same NIC)
══════════════════════════════════════════════════════════════════════════════
```

---

## 2. Module Definitions

Three logical modules. Threads vs code-files is an implementation detail; the logical separation must hold regardless.

### 2.1 Aurora Adapter (`bridge/include/aurora_adapter.h`)

**Responsibilities:**
- Load and validate field-map YAML (`config/aurora_fieldmap.example.yaml` or operator-specified path).
- Open `/dev/uio0`, `mmap` FIFO BAR at `0xA0010000` (base address from `design_1.hwh`, M2-A §2.1).
- Run T1 (Aurora RX Worker): `poll(/dev/uio0)` → drain 37 words from FIFO (`RDFD`, `RDFO`, `RLR` registers — PG080 verified offsets in M2-A §11.4) → decode via field-map → invoke `on_frame` callback → `write(uio_fd, &one_u32, 4)` to re-arm UIO.
- Run TX path (from T4 caller): assemble 23-word frame from `aurora_tx_field_t[]` per YAML TX schema → write to `TDFD` → set `TLR`.
- Provide `aurora_adapter_get_stats()` for T5 supervisor.

**Interface contract:** Fully defined in `bridge/include/aurora_adapter.h`. Stable. Not changing.

**Current state:** Stub. Every function returns `AURORA_ERR_NOT_IMPLEMENTED`. Implementation is M3.

**Critical constraint from M2-A §2.2 (Aurora adapter header concurrency model):**
> The `on_frame` callback runs in the adapter's RX thread, NOT in interrupt context. Callers must not block in the callback; instead they post to the next stage via a lock-free latest-value buffer.

The `on_frame` callback is T1's only output. It must complete in well under one Aurora inter-frame interval (order of 125 µs assumed; exact value is M2-A Gap 10.2). In practice the callback needs only to lock a mutex, memcpy 37 decoded field values, update state/timestamp, signal T2, and release the mutex — this is microsecond-range, safe.

### 2.2 Translation Core / Canonical Buffer

**Responsibilities:**
- Own two canonical buffers (structs defined in §4 below): `aurora_to_egd_buf` and `egd_to_aurora_buf`.
- Provide the `EMPTY / VALID / STALE / INVALID` state model from `DESIGN_INTENT.md`.
- Implement freshness checking (T5 supervisor sets STALE when `now_ns - last_rx_ns > stale_threshold_ns`).
- Implement the field-name-keyed lookup so T2 and T4 can read/write by field name (from YAML) rather than word index.
- **No protocol logic.** The translation core does not know what "Aurora" or "EGD" is — it just stores typed values by name.

**What translation does NOT do:** It does not re-implement `Axi_IO.c`'s direct struct writes (`PEBB_Control_U.feedback[0]`, etc.). The aurora_adapter decodes the raw FIFO words into named `aurora_field_t` values using the YAML scale factors; the translation core just moves those named values between two buffers. The EGD adapter maps buffer field names to EGD JSON keys.

### 2.3 EGD Adapter

**Responsibilities:**
- At startup: fetch EGD config XML (HTTP GET, port 7937) → build `EgdPageInfo` / `var_map` (see §5 for the 6 decisions from M2-B §9).
- Run T2 (EGD TX Publisher): on T1's condvar signal OR periodic timer (whichever comes first) → lock `aurora_to_egd_buf` → check validity → build JSON string keyed by EGD field names → `JsonClient::Publish()` → release lock.
- Register `JsonClient::Subscribe()` which spawns T3 (libEGD-internal): callback writes to `egd_to_aurora_buf`, signals T4.
- Provide a clean `egd_adapter_stop()` that calls `JsonClient` destructor (which joins T3) then releases `EgdPageInfo`.

---

## 3. Threading Model — Recommendation and Rationale

**Chosen model: Multi-thread, event-driven.** Not single-thread round-robin. Not sidecar process (for initial implementation).

### 3.1 Why not single-thread round-robin

Aurora RX is interrupt-driven — the RC IRQ fires at the RTDS model rate (order of 8 kHz; exact value unsourced, M2-A Gap 10.2). A single-threaded round-robin loop would need to poll both `/dev/uio0` (Aurora) and the EGD socket in one `select()`/`epoll()` call. The problem is the Aurora frame must be drained promptly after the RC IRQ fires — any latency in the loop adds to inter-frame jitter and can cause `AURORA_ERR_FIFO_OVERRUN` (new RC IRQ while the previous frame is still being drained). The `aurora_adapter.h` concurrency model already commits to a dedicated RX thread; there is no single-thread option without redesigning the adapter header.

### 3.2 Why not sidecar process (for now)

`BRIDGE_SOFTWARE_DESIGN.md` §4.3 listed this as Option B. The reasons to defer it:

- The `aurora_adapter.h` contract is designed for in-process use (opaque handle, callback into caller's context). Wrapping that in IPC adds a serialisation/deserialisation layer with no benefit at this stage.
- Shared memory + message queues are more complex to get right than a mutex inside one process, and the `DESIGN_INTENT.md` explicitly says "prefer deterministic behavior and simplicity, avoid unnecessary framework complexity".
- The sidecar pattern becomes attractive if (a) the controller's CHIL firmware needs to run as a separate binary that cannot be modified, or (b) fault isolation requirements demand process-level restart. Neither applies today.

**Record for the reviewer:** If the production system requires process isolation (e.g. the CHIL code must keep running if the EGD stack crashes), revisit sidecar in M3. The aurora_adapter interface is thin enough that wrapping it in a socket/pipe shim later is feasible.

### 3.3 Thread responsibilities summary

| Thread | Name | Wakeup mechanism | Blocking operations | Output |
|---|---|---|---|---|
| T1 | Aurora RX Worker | `poll(/dev/uio0)` — blocked until RC IRQ | UIO poll (blocks), FIFO word reads (brief MMIO) | Writes `aurora_to_egd_buf`, signals T2 |
| T2 | EGD TX Publisher | condvar from T1 OR `clock_nanosleep` timer | `JsonClient::Publish()` → `sendto()` (brief) | UDP datagram on wire |
| T3 | EGD RX (libEGD) | `poll(socket)` — managed by libEGD | UDP recv (blocks inside libEGD) | Fires callback → writes `egd_to_aurora_buf`, signals T4 |
| T4 | Aurora TX Worker | condvar from T3 callback | `aurora_adapter_send_tx()` → FIFO MMIO writes (brief) | TX FIFO write → Aurora PL |
| T5 | Supervisor | `clock_nanosleep` ~100 ms | None (non-blocking reads of stats + buffer state) | Log output, health file, STALE state transitions |

### 3.4 Blocking vs non-blocking — boundary rules

| Location | Rule | Reason |
|---|---|---|
| T1: `poll()` | Blocking — correct | Dedicated thread; nothing else runs on T1 |
| T1: `on_frame` callback | **Must NOT block** | Delays UIO re-arm → next RC IRQ missed → `AURORA_ERR_FIFO_OVERRUN` |
| T2: `JsonClient::Publish()` | Sync `sendto()` — brief, bounded | Socket write completes in microseconds on LAN; acceptable |
| T2: mutex acquisition on `aurora_to_egd_buf` | Mutex — must be brief | T1 holds this mutex during write; keep critical section short |
| T3 callback (libEGD) | **Must NOT block** | Runs on libEGD's internal thread; blocking starves next EGD packet |
| T4: `aurora_adapter_send_tx()` | FIFO MMIO write — brief, bounded | 23 × 32-bit writes; microsecond-range |
| T5: all reads | Non-blocking | Supervisor is monitoring only; MUST NOT compete with data path |

### 3.5 Locking strategy

Start simple per `DESIGN_INTENT.md`:
- One `pthread_mutex_t` per canonical buffer (`aurora_to_egd_buf_mutex`, `egd_to_aurora_buf_mutex`).
- Critical sections: T1 write (lock, memcpy 37 values, update state+ts, unlock) and T2 read (lock, snapshot 37 values + state, unlock). Order-of-magnitude: < 5 µs hold time for 37 × 8-byte copies on A53 @ 1.5 GHz.
- One `pthread_cond_t` per buffer (`aurora_to_egd_cond`, `egd_to_aurora_cond`).
- Do NOT use lock-free structures in M3 initial implementation. Profile first; optimize only if measurements show contention.

---

## 4. Canonical Buffer Specification

### 4.1 `aurora_to_egd_buf` — Aurora RX → EGD TX

```c
/* Lives in translation core. Owned by process; shared between T1 (writer)
 * and T2 (reader). T5 reads state (read-only, under mutex). */

typedef enum {
    BUF_EMPTY   = 0,  /* no frame received since startup */
    BUF_VALID   = 1,  /* latest frame passed validation */
    BUF_STALE   = 2,  /* last_rx_ns is older than stale_threshold_ns */
    BUF_INVALID = 3   /* latest frame failed validation (NaN / length mismatch) */
} buf_state_t;

/* One decoded field, matching aurora_field_t from aurora_adapter.h */
typedef struct {
    char         name[64];       /* from YAML field name */
    double       value;          /* decoded + scaled local value */
    int          is_nan;         /* 1 if the wire value was NaN */
} canon_field_t;

#define AURORA_RX_FIELD_COUNT  37  /* RECEIVE_LENGTH from Axi_IO.h */
#define AURORA_TX_FIELD_COUNT  23  /* TRANSMIT_LENGTH from Axi_IO.h */

typedef struct {
    canon_field_t   fields[AURORA_RX_FIELD_COUNT];
    buf_state_t     state;
    uint64_t        last_rx_ns;      /* CLOCK_MONOTONIC at RC IRQ wakeup */
    uint32_t        seq_last;        /* last seen sequence number (word 36) */
    uint64_t        frames_ok;       /* diagnostic: copied from adapter stats */
    uint64_t        frames_invalid;
    pthread_mutex_t mutex;
    pthread_cond_t  updated;         /* signalled by T1 after each write */
} aurora_to_egd_buf_t;
```

### 4.2 `egd_to_aurora_buf` — EGD RX → Aurora TX

```c
typedef struct {
    canon_field_t   fields[AURORA_TX_FIELD_COUNT];
    buf_state_t     state;
    uint64_t        last_rx_ns;
    pthread_mutex_t mutex;
    pthread_cond_t  updated;         /* signalled by T3 callback after each EGD RX */
} egd_to_aurora_buf_t;
```

### 4.3 Buffer state transitions

```
                    ┌── T1 writes valid frame ──►  VALID
                    │                                │
  EMPTY ────────────┤                                ├── T5: elapsed > threshold ──► STALE
  (initial)         │                                │
                    └── T1 writes invalid frame ──► INVALID     STALE ────────────► VALID
                                                      │                              (T1 writes
                                                      └────── T1 recovers ──────────  valid frame)
```

**What T2 does based on state:**

| `aurora_to_egd_buf.state` | T2 action |
|---|---|
| `BUF_EMPTY` | Do NOT publish. Log once. |
| `BUF_VALID` | Publish with current field values. |
| `BUF_STALE` | Publish with last known values AND set EGD `Status` field to `EGD_MESSAGE_UNHEALTHY` (= 1 per EGD spec). Log warning. |
| `BUF_INVALID` | Do NOT publish. Log error. If persistent, transition to FAULT state. |

### 4.4 Freshness threshold

Default: `stale_threshold_ns = 500 * 1000 * 1000` (500 ms). Configurable in the startup config file.

Rationale: Aurora inter-frame interval is expected to be order-of-magnitude 125 µs (unsourced — M2-A Gap 10.2). Even at 10× that (1.25 ms), 500 ms gives > 400 missed frames before STALE. This is deliberately conservative so a brief network blip does not immediately degrade service. Tighten after board measurements close Gap 10.2.

---

## 5. EGD Config Fetch — Startup Sequence and the 6 Design Decisions

These are the 6 open items from `M2-B_libegd_findings.md` §9 (handoff to M2-D). All 6 are decided here.

### 5.1 Design decisions

| # | Decision | Chosen approach | Rationale |
|---|---|---|---|
| 9.1 | Config server address | **Config file** (`bridge.yaml` alongside the field-map) — keys: `egd.producer_host`, `egd.producer_port`, `egd.exchange_id`. Not hard-coded; not CLI arg (too fragile in systemd unit). | Same pattern as the field-map YAML: one config file the operator edits for their deployment. |
| 9.2 | Retry on fetch failure | **Exponential back-off, max 5 retries** (1 s, 2 s, 4 s, 8 s, 16 s), then FAULT and exit. Fail-closed: a partial `var_map` is worse than no var_map. | DESIGN_INTENT: predictable degraded-state behavior. A bridge that starts with the wrong field mapping silently corrupts data. |
| 9.3 | Startup ordering | **EGD config fetch → validate var_map → `aurora_adapter_create()` → `aurora_adapter_start()` (T1) → EGD subscribe (T3) → EGD TX timer (T2) → Aurora TX worker (T4) → Supervisor (T5).** | If config fetch fails, no hardware I/O is attempted. UIO open failure does not block config fetch. Clean unwind on any step failure. |
| 9.4 | `ProducerID` derivation | **Explicit in config file** (`egd.producer_ip`). Do NOT auto-detect from NIC. | Multi-homed Kria (management NIC + data NIC) makes auto-detection ambiguous. Explicit is safer. |
| 9.5 | Directions to fetch | **Bidirectional by default**: fetch `Type=ProducedData` (EGD the bridge produces, i.e. Aurora→EGD direction) AND `Type=ConsumedData` (EGD the bridge consumes, i.e. EGD→Aurora direction). Configurable via `egd.subscribe_consumed: true/false` for deployments where the reverse direction is not used. | DESIGN_INTENT says bidirectional. Allow disabling the reverse direction for read-only deployments. |
| 9.6 | Config XML caching | **Cache to disk** at `bridge_cache/egd_config.xml` on first successful fetch. On subsequent startups, if fetch fails all retries but cache file exists and is < `config_cache_max_age_hours` (default: 24 h), use the cached copy with a prominent WARNING log line. After `config_cache_max_age_hours`, do NOT use cache — require a fresh fetch or fail-closed. | Balances reliability (brief EMS server downtime does not prevent bridge restart) vs safety (stale config silently misroutes fields). |

### 5.2 Startup sequence diagram

```
main()
  │
  ├─ load bridge.yaml (config: EGD host, thresholds, paths)
  │    └─ on error → log + exit(1)
  │
  ├─ fetch EGD config XML  (HTTP GET, port 7937, max 5 retries with back-off)
  │    ├─ on success → cache to disk, build var_map
  │    └─ on all retries failed →
  │         ├─ if cache file exists and < max_age → use cache + WARN
  │         └─ else → log FAULT + exit(1)
  │
  ├─ validate var_map (field names match YAML TX/RX schema; count matches 37 / 23)
  │    └─ on mismatch → log schema error + exit(1)
  │
  ├─ aurora_adapter_create(yaml_path)
  │    └─ on AURORA_ERR_CONFIG_* → log + exit(1)
  │
  ├─ aurora_to_egd_buf_init() + egd_to_aurora_buf_init()  (mutexes + condvars)
  │
  ├─ aurora_adapter_start(uio_path, on_frame_callback, &aurora_to_egd_buf)
  │    └─ on AURORA_ERR_UIO_OPEN / AURORA_ERR_FIFO_MMAP → log + exit(1)
  │    T1 is now running and waiting for first RC IRQ.
  │
  ├─ JsonClient::Subscribe(egd_host, egd_port, consumed_page_info, on_egd_rx, &egd_to_aurora_buf)
  │    T3 (libEGD-internal) is now running.
  │
  ├─ pthread_create(&t2, NULL, egd_tx_publisher_thread, ...)
  │    T2 starts; immediately blocks on condvar (aurora_to_egd_cond).
  │
  ├─ pthread_create(&t4, NULL, aurora_tx_worker_thread, ...)
  │    T4 starts; immediately blocks on condvar (egd_to_aurora_cond).
  │
  ├─ pthread_create(&t5, NULL, supervisor_thread, ...)
  │    T5 starts; enters sleep loop.
  │
  └─ pthread_join(t2, NULL)  ← main thread waits; process stays alive
     (on SIGTERM → set stop_flag → all threads check flag → clean join)
```

---

## 6. Failure Modes and Degraded-State Behavior

| Failure | Detection | Bridge response |
|---|---|---|
| Aurora link down (`channel_up = 0`) | `aurora_to_egd_buf.last_rx_ns` ages past `stale_threshold_ns`; T5 sets `state = BUF_STALE` | T2 publishes UNHEALTHY EGD (Status=1); Aurora TX continues with last-known egd_to_aurora values. Log warning every N seconds. |
| Aurora frame length mismatch (got ≠ 37 words) | `AURORA_ERR_FIFO_LENGTH_MISMATCH` in T1; adapter increments `rx_frames_length_mismatch` counter | T1 discards frame; `aurora_to_egd_buf.state = BUF_INVALID`; T2 stops publishing until next valid frame |
| NaN in Aurora RX float field | `AURORA_ERR_NAN_IN_FRAME` in T1 / `is_nan` flag in decoded field | T1 invokes callback with `is_nan` set; canonical buffer marks that field invalid; T2 publishes with that field omitted from JSON (EGD consumer sees no update for that field) |
| EGD network unreachable | `sendto()` returns error in T2 | Log error; T2 continues attempting. EGD is UDP/fire-and-forget — there is no TCP connection to manage. Increment an EGD TX error counter for T5 to report. |
| EGD RX absent (no EMS sending commands) | `egd_to_aurora_buf.last_rx_ns` ages past threshold | T5 sets `egd_to_aurora_buf.state = BUF_STALE`; T4 continues sending last-known Aurora TX values (safe — RTDS would observe a frozen command set, not garbage) |
| EGD Signature mismatch | libEGD reports `EGD_MESSAGE_SIG_ERROR` in T3 callback | T3 callback discards message; increment counter; if persistent → log FAULT (possible config version mismatch) |
| UIO device disappears (DTBO unloaded mid-run) | `poll()` returns error in T1 | T1 logs FATAL; sets `stop_flag`; all threads clean up; process exits with non-zero. Operator must reload bitstream and restart. |
| YAML field-map config mismatch vs EGD var_map | Detected at startup (§5.2 validation step) | Exit before any hardware I/O. |
| Sequence number gap in Aurora RX (comm_glitch) | T1 detects `seq != seq_prev + 1` (mod 1000); increments `commGlitchCtr` (matches `Axi_IO.c` `COMM_GLITCH_LIMIT = 10`) | If glitch count exceeds `comm_glitch_threshold` (configurable; default 10 per `aurora_fieldmap.example.yaml`) → set `CHIL_error = 1` in TX frame; T2 logs comm_glitch counter |

---

## 7. Per-Boundary Data-Transfer Table

| Boundary | From | To | Mechanism | Buffer | Sync | Notes |
|---|---|---|---|---|---|---|
| **B1** Aurora RX | T1 (Aurora RX Worker) after UIO wakeup | `aurora_to_egd_buf` | `aurora_on_frame_cb_t` invocation | struct copy (37 × 8 bytes = 296 bytes) | `pthread_mutex_t`, held during write | Callback MUST NOT block. Runs in T1 context. |
| **B2** Aurora → EGD | `aurora_to_egd_buf` | T2 (EGD TX Publisher) | condvar signal OR timer wakeup | Snapshot: copy 37 values + state under mutex (read side) | `pthread_mutex_t`, held during snapshot | T2 works on the snapshot; lock is released before `JsonClient::Publish()` |
| **B3** EGD TX wire | T2 | UDP socket | `JsonClient::Publish()` → `sendto()` | 1400-byte datagram buffer (stack or libEGD-internal) | None (synchronous, fire-and-forget) | EGD spec: UDP port 18246, `PDUType = 13` |
| **B4** EGD RX wire | UDP socket | T3 (libEGD RX) | `recvfrom()` in libEGD's `PollSocket()` | 1428-byte recv buffer (28-byte hdr + 1400-byte payload) | libEGD-internal | T3 validates header (PDUType, ProducerID, ExchangeID, Signature) |
| **B5** EGD → Aurora | T3 callback (libEGD) after JSON decode | `egd_to_aurora_buf` | `on_egd_rx_callback()` | struct copy (23 × 8 bytes = 184 bytes) | `pthread_mutex_t` held during write | Callback MUST NOT block. Signal T4 condvar. |
| **B6** EGD → Aurora TX | `egd_to_aurora_buf` | T4 (Aurora TX Worker) | condvar signal | 23-field snapshot under mutex | condvar + mutex | T4 builds `aurora_tx_field_t[]` from snapshot, calls `aurora_adapter_send_tx()` |
| **B7** Aurora TX FIFO | T4 | AXI FIFO MMIO (PL) | `aurora_adapter_send_tx()` → 23 × `TDFD` writes + `TLR` | PG080 TX FIFO (depth per BD) | None (MMIO writes, uncached) | T4 must NOT call `send_tx()` simultaneously with T1's FIFO drain. The adapter owns the FIFO descriptor; concurrent calls are serialised inside the adapter. |

---

## 8. State Machine (Bridge Process Level)

```
  STARTUP
     │
     ├─ config/YAML load OK → EGD fetch OK → aurora_adapter_create OK
     │
     ▼
  ACTIVE  ◄─────────────────────────────────────────────┐
     │                                                    │
     ├─ aurora_to_egd_buf goes STALE (T5) ────────►  DEGRADED ─────────────────────► ACTIVE
     │                                                    │     (T1 delivers valid frame,
     │                                                    │      state transitions back)
     ├─ repeated INVALID frames (threshold) ─────────────┘
     │
     ├─ UIO device disappears ──────────────────────────────────────────────────► FAULT
     ├─ EGD var_map mismatch ──────────────────────────────────────────────────► FAULT
     ├─ EGD config fetch: all retries + cache expired ─────────────────────────► FAULT
     └─ aurora_adapter_start() returns hardware error ──────────────────────────► FAULT
                                                                                    │
                                                                               exit(non-zero)
                                                                            (systemd can restart)
```

**Safe outputs in DEGRADED:** Aurora TX continues sending last-known values. EGD TX publishes with `Status = EGD_MESSAGE_UNHEALTHY`. Neither side gets garbage — they get the last known valid state.

**In FAULT:** Clean shutdown: `aurora_adapter_stop()` → `egd_adapter_stop()` → buffer destroy → exit. Systemd can restart the process after a configurable delay.

---

## 9. Configuration File Schema (`bridge.yaml`)

This file sits alongside `aurora_fieldmap.example.yaml`. The bridge process reads both at startup.

```yaml
schema_version: "1.0"

aurora:
  uio_device:        "/dev/uio0"          # path to UIO device for AXI FIFO
  field_map:         "config/aurora_fieldmap.example.yaml"
  stale_threshold_ms: 500                  # STALE if no valid frame in this window
  comm_glitch_threshold: 10               # matches aurora_fieldmap.yaml validation section

egd:
  producer_host:     "192.168.1.50"        # EGD producer IP — used for config fetch + bind
  producer_port:     18246                  # data port (EGD spec: 18246)
  config_port:       7937                   # HTTP config port (EGD spec: 7937)
  exchange_id:       1                      # ExchangeID to subscribe to
  producer_ip:       "192.168.1.50"        # explicit ProducerID (see M2-B §9.4)
  subscribe_consumed: true                  # fetch + subscribe to ConsumedData (EGD→Aurora)
  config_cache_path: "bridge_cache/egd_config.xml"
  config_cache_max_age_hours: 24
  stale_threshold_ms: 2000                  # STALE for egd_to_aurora_buf

supervisor:
  period_ms:         100                    # T5 wake interval
  log_stats_every_n: 10                    # log stats every N supervisor wakeups
  health_file:       "/tmp/bridge_health"  # written by T5; optional
```

---

## 10. Open Risks

| # | Risk | Severity | Mitigation |
|---|---|---|---|
| R1 | **Gap 10.8 (Aurora lab variant unvalidated):** The 156.25 MHz / 2.5 Gbps lab-variant build has not been elaborated in Vivado. Software path is identical between lab variant and production; hardware-level Aurora train may not come up. | High | Closes when M2-A §11 stage S3 runs on Kria. All M2-D software is rate-independent. |
| R2 | **`aurora_adapter.c` is a stub:** No hardware I/O, no UIO, no YAML loader in committed code. | High | This is the primary M3 task (see §11 below). Known and explicitly documented. |
| R3 | **Aurora frame rate unsourced (M2-A Gap 10.2):** The `stale_threshold_ms = 500` default is conservative. If the real rate is slower, the threshold may need loosening. | Medium | Will self-correct when stage S3 board measurement fills Gap 10.2. The threshold is configurable in `bridge.yaml`. |
| R4 | **EGD config XML format for this deployment not verified:** M2-B documents the libEGD XML format; we have not seen the actual ControlST server's output for this customer. Field names in the config XML must match the YAML field-map names for the adapter to route correctly. | Medium | Verify the actual XML against `aurora_fieldmap.example.yaml` field names when EMS access is available. |
| R5 | **libEGD ARM64 runtime exercise not done:** M2-C confirmed ELF format + ABI only (no running EGD round-trip on A53). | Medium | Smoke test in M2-A §11.6; full EGD round-trip in M3 integration test. |
| R6 | **Aurora TX timing relative to EGD RX:** T4 sends a TX frame for every EGD RX message. If the EGD rate > Aurora's TX capacity, T4 may queue faster than Aurora can consume. The adapter has no internal TX queue — `aurora_adapter_send_tx()` is synchronous (writes FIFO directly). | Low | EGD RX rate is expected << Aurora link capacity. If this ever becomes a problem, add a rate-limiter in T4 (drop or coalesce). |
| R7 | **DTBO generation (M2-A Gap 10.6):** Without the device-tree overlay, `/dev/uio0` does not exist. `aurora_adapter_start()` returns `AURORA_ERR_UIO_OPEN`. | Medium | Gated on Gap 10.4 (Vivado run). Not a bridge-software issue; documented here so the M3 bringup engineer knows the dependency. |

---

## 11. M3 Implementation Tasks

These are the concrete coding tasks that land in M3. M2-D is the **design**; M3 is the **implementation**.

| # | Task | Owner module | Depends on |
|---|---|---|---|
| M3.1 | Implement `aurora_adapter.c`: YAML loader (libyaml or similar), UIO open + mmap, T1 RX thread (poll → drain → decode → callback), T send path (pack → TDFD × 23 → TLR), `get_stats()`. | `bridge/src/aurora_adapter.c` | Gap 10.4 (Vivado run + DTBO) |
| M3.2 | Implement canonical buffer types + init/destroy functions. | `bridge/src/canonical_buffer.c` (new) | None |
| M3.3 | Implement `on_frame` callback (T1 output): lock `aurora_to_egd_buf`, memcpy `aurora_field_t[]` to `canon_field_t[]`, update state/ts, signal T2 condvar. | `bridge/src/bridge_main.c` or `aurora_rx_handler.c` | M3.1, M3.2 |
| M3.4 | Implement T2 (EGD TX Publisher thread): condvar wait, snapshot buffer, build JSON string, `JsonClient::Publish()`. | `bridge/src/egd_tx.c` (new) | M3.2, M2-C libEGD |
| M3.5 | Implement EGD config fetch at startup: HTTP GET, retry logic, disk cache. | `bridge/src/egd_config_fetch.c` (new) | libcurl (already in M2-C cross-build) or simple socket HTTP |
| M3.6 | Implement EGD RX subscribe + callback (`on_egd_rx`): write to `egd_to_aurora_buf`, signal T4. | `bridge/src/egd_rx.c` (new) | M3.2, M2-C libEGD |
| M3.7 | Implement T4 (Aurora TX Worker thread): condvar wait, snapshot buffer, pack `aurora_tx_field_t[]`, call `aurora_adapter_send_tx()`. | `bridge/src/aurora_tx.c` (new) | M3.1, M3.2, M3.6 |
| M3.8 | Implement T5 (Supervisor thread): freshness check, STALE transitions, stats logging, health file. | `bridge/src/supervisor.c` (new) | M3.2, M3.1 |
| M3.9 | Implement `main()`: read `bridge.yaml`, startup sequence (§5.2), signal handlers (SIGTERM → clean shutdown), thread joins. | `bridge/src/bridge_main.c` | M3.1–M3.8 |
| M3.10 | CMakeLists.txt for bridge binary targeting ARM64: link against `out/arm64/lib/libEGD.so`, libyaml, libpthread. | `bridge/CMakeLists.txt` | M2-C cross-toolchain |
| M3.11 | Unit tests (host, not ARM64): canonical buffer state transitions, T1 callback with mock frame, T2 publish with mock `JsonClient`, startup ordering. | `bridge/test/` | M3.2–M3.6 |
| M3.12 | Integration test on A53: load bitstream (stage S3), start bridge process, inject pattern from pattern generator, verify EGD datagram on second host (`tcpdump`). | Lab hardware + M3.1–M3.9 | M2-A S3, board access |

---

## 12. Cross-Reference to M2-A/B/C Findings

This section explicitly verifies no contradictions exist.

| Claim in this doc | Source in M2-A/B/C | Consistent? |
|---|---|---|
| AXI FIFO base `0xA0010000` | M2-A §2.1, `design_1.hwh` `axi_fifo_mm_s_0.baseaddr` | ✓ |
| FIFO register offsets (IER=0x04, RDFD=0x20, TLR=0x14, etc.) | M2-A §11.4, PG080 v4.2 verified | ✓ |
| RC IRQ = `XLLF_INT_RC_MASK = 0x04000000` (bit 26) | M2-A §4, PG080 ISR bit definitions | ✓ |
| UIO re-arm: `write(uio_fd, &one_u32, 4)` before next `poll()` | M2-A §11.4 critical detail | ✓ |
| RX 37 words (`RECEIVE_LENGTH = 37`) | `Axi_IO.h` line 23; YAML `rx.word_count = 37` | ✓ |
| TX 23 words (`TRANSMIT_LENGTH = 23`) | `Axi_IO.h` line 22; YAML `tx.word_count = 23` | ✓ |
| EGD port 18246 | M2-B §1, `egd_spec.h` | ✓ |
| EGD config port 7937 | M2-B §2, `README.md` | ✓ |
| `JsonClient` recommendation | M2-B §8, maintained | ✓ |
| `on_frame` callback MUST NOT block | `aurora_adapter.h` concurrency model comment | ✓ |
| libEGD subscriber callback MUST NOT block | M2-B §4, threading model | ✓ |
| `libEGD.so` at `out/arm64/lib/libEGD.so` | M2-C cross-compile output path | ✓ |
| Gap 10.8 (lab variant) pending Vivado validation | M2-A §10.8 current status | ✓ — software path is rate-independent |
| Gap 10.2 (frame rate unsourced) | M2-A §10.2 | ✓ — thresholds configurable |
| Gap 10.5 (aurora_adapter stub) | M2-A §10.5, aurora_adapter.c | ✓ — explicitly M3 |
| aurora_adapter.h interface contract | M2-A §7, committed header | ✓ — no change |
| EMPTY/VALID/STALE/INVALID states | `DESIGN_INTENT.md` §Buffer State Model | ✓ |
| Producer/consumer decoupling | `DESIGN_INTENT.md` §Runtime Dataflow Model | ✓ |
| "Do not embed Aurora framing in EGD" | `BRIDGE_SOFTWARE_DESIGN.md` §3 | ✓ |

No contradictions found. All M2-A/B/C open items that are relevant to software architecture are resolved here; those requiring hardware (Gap 10.4/10.3/10.8) remain open and are listed in §10 Open Risks.

---

## 13. Document Control

| Version | Date | Notes |
|---|---|---|
| 1.0 | 2026-05-27 | Initial M2-D deliverable. Architecture designed; implementation is M3. |

**This document closes the M2-D acceptance criteria as follows:**

| Criterion | Status |
|---|---|
| Architecture diagram (ASCII) | ✓ §1 |
| Threading model recommendation + rationale | ✓ §3 |
| Data-transfer table per boundary | ✓ §7 |
| Open risks and M3 implementation tasks | ✓ §§10, 11 |
| Consistent with M2-A/B/C findings | ✓ §12 |
