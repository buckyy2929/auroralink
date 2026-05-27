# M2-D — Bridge Application Architecture: पूरी समझ (Hindi में)

### Abhishek के लिए — Border Station Translator metaphor से, simple language में

> **यह document `M2-D_application_design.md` का Hindi mirror है।**  
> English doc reviewer/customer के लिए है। यह Hindi doc तुम्हारे खुद के लिए है — हर concept train/border-station example से, step-by-step।

---

## ⚠ पहले honest status

**Bridge software कहाँ है आज:**

| Item | Status |
|---|---|
| `bridge/include/aurora_adapter.h` | ✅ Complete — interface contract defined |
| `config/aurora_fieldmap.example.yaml` | ✅ Complete — 37 RX + 23 TX fields, Axi_IO.c से exact |
| `bridge/src/aurora_adapter.c` | ❌ STUB — सब functions `NOT_IMPLEMENTED` return करते हैं |
| Architecture design (M2-D_application_design.md) | ✅ Done (यह document उसी का Hindi mirror है) |
| Actual running bridge on Kria | ❌ M3 में implement होगा |

**Reviewer से क्या कहना है:** "Architecture designed, interface contracted, M3 में implement होगा — board पर run नहीं हुआ अभी।"

---

## 🚉 पहले एक कहानी — "Border Station Translator"

> कल्पना करो: दो देशों के बीच एक **Border Railway Station** है।
>
> **Left side — Train Country (Aurora):**
> - RTDS (दूसरा देश) हर 125 µs में एक **Train** (data packet) भेजता है।
> - हर train में **37 Wagons** (37 words = 37 × 4 bytes) होते हैं।
> - Wagons में voltages, currents, commands — सब कुछ binary में encode है।
> - Train बहुत fast आती है — fiber optic track पर।
>
> **Right side — Radio Country (EGD):**
> - EGD network पर **Radio Announcements** broadcast होती हैं।
> - हर announcement में field names और values होती हैं — JSON format में।
> - EGD rate slow है — हर 10–100 ms में एक announcement।
>
> **Border Station पर Translator बैठा है — वो हमारा Bridge है।**
>
> Bridge के पाँच workers (threads) हैं — हर एक का specific काम है।

---

## 📦 Part 1: Bridge के पाँच Workers (5 Threads)

```
Worker T1: "Wagon Receiver" (Aurora RX)
  - Platform पर बैठा है
  - जैसे ही train आती है (UIO interrupt), wagons उठाता है
  - Decoded values को notebook (canonical buffer) में लिखता है
  - T2 को bell बजाता है "नया data आया"

Worker T2: "Radio Announcer" (EGD TX Publisher)
  - Radio room में बैठा है
  - T1 की bell सुनते ही (या हर X ms पर) notebook पढ़ता है
  - EGD message construct करके UDP पर broadcast करता है

Worker T3: "Radio Listener" (EGD RX — libEGD manages this)
  - Radio room में दूसरा earpiece लगाए बैठा है
  - EGD से commands सुनता है (EMS system से)
  - Decoded values को दूसरी notebook (reverse buffer) में लिखता है
  - T4 को bell बजाता है "नया command आया"

Worker T4: "Wagon Packer" (Aurora TX)
  - T3 की bell सुनते ही reverse notebook से values उठाता है
  - 23-wagon train pack करता है
  - Aurora TX FIFO में भेजता है (RTDS को reply)

Worker T5: "Station Master" (Supervisor)
  - हर 100 ms में check करता है
  - "क्या last train 500 ms से ज़्यादा पहले थी? → STALE!"
  - Counters log करता है, health file update करता है
```

---

## 📦 Part 2: Architecture — पूरा Picture एक नज़र में

```
RTDS (दूसरा देश)
    │ fiber track
    ▼
[Aurora IP, FPGA PL]   ◄────────────────────────────── T4 TX FIFO write
    │ AXI-Stream                                                ▲
    ▼                                                           │
[AXI FIFO, FPGA PL]                                      [egd_to_aurora_buf]
    │ RC IRQ → /dev/uio0                                    ▲
    ▼                                                       │
[T1: Aurora RX Worker]                              [T3 EGD RX callback]
    │ decode 37 words                                       │
    │ scale factors apply                              [libEGD T3 thread]
    ▼                                                       ▲
[aurora_to_egd_buf]                                         │
    │                                                [JsonClient::Subscribe]
    ▼                                                       ▲
[T2: EGD TX Publisher]                                      │
    │ build JSON                                         UDP 18246
    │ JsonClient::Publish()                              EGD network
    ▼
UDP 18246 → EGD network → EMS subscribers

                                   [T5: Supervisor]
                                   checks freshness, logs, STALE transitions
```

---

## 📦 Part 3: दो Notebooks (Canonical Buffers)

Notebook का मतलब है **Canonical Buffer** — एक struct जो field values + state + timestamp store करता है।

### Notebook A: `aurora_to_egd_buf` (RTDS → EGD direction)

```
┌─────────────────────────────────────────────────────────┐
│  aurora_to_egd_buf                                      │
│                                                         │
│  fields[37]:                                            │
│    [0]  key             = 12345  (s32)                  │
│    [1]  i_high          = 1250.5 A  (float64)           │
│    [2]  vhigh_B         = 380000 V  (float64)           │
│    ...                                                  │
│    [36] seq             = 487     (s32)                 │
│                                                         │
│  state:    EMPTY | VALID | STALE | INVALID              │
│  last_rx_ns: 1716808450_123456789  (CLOCK_MONOTONIC)    │
│  mutex:    (lock before read/write)                     │
│  condvar:  (T1 signals T2 after write)                  │
└─────────────────────────────────────────────────────────┘
```

### Notebook B: `egd_to_aurora_buf` (EGD → Aurora TX direction)

```
┌─────────────────────────────────────────────────────────┐
│  egd_to_aurora_buf                                      │
│                                                         │
│  fields[23]:                                            │
│    [0]  phaseA_duty  = 0.42  (float64)                  │
│    [2]  packed_bits  = 0x07  (u32)                      │
│    [10] next_state   = 3     (s32)                      │
│    ...                                                  │
│                                                         │
│  state:    EMPTY | VALID | STALE | INVALID              │
│  last_rx_ns: 1716808450_987654321                       │
│  mutex, condvar (T3 signals T4 after write)             │
└─────────────────────────────────────────────────────────┘
```

### Buffer State — "Notebook का हाल"

```
EMPTY   → पहली entry आई नहीं (startup पर)
VALID   → data fresh है, use करो
STALE   → last entry 500 ms से पुरानी है — data old हो गया
INVALID → last frame NaN/length error था — data garbage
```

**Train analogy:**
- `VALID` = "Platform पर fresh train है, wagons उठाओ"
- `STALE` = "500 ms से कोई train नहीं आई — शायद track problem?"
- `INVALID` = "Train आई लेकिन wagons टूटे हुए थे — use मत करो"
- `EMPTY` = "सुबह हुई, अभी पहली train आई नहीं"

---

## 📦 Part 4: T1 Worker — Wagon Receiver का काम (Aurora RX)

### Step-by-step:

```
1. T1 starts → poll(/dev/uio0)   ← BLOCKED — train का wait करो
   
2. Train आई! Aurora IP ने RC IRQ fire किया।
   
3. T1 wakes up:
   a. XLlFifo_iRxGetLen() → check: क्या exactly 37 words आए?
   b. अगर नहीं → AURORA_ERR_FIFO_LENGTH_MISMATCH
      → aurora_to_egd_buf.state = BUF_INVALID
      → goto step 6
   
4. 37 words drain करो (RDFD register से एक-एक word read):
   for i in 0..36:
     raw_word = FIFO[RDFD]
     field = yaml_field_map[i]
     if field.wire_type == float32:
         value = reinterpret_as_float(raw_word) * field.scale
         if is_nan(value): mark field.is_nan = 1
     else:
         value = raw_word  (as s32/u32, scale = 1.0)
     decoded[i] = {name, value, is_nan}
   
5. Mutex lock aurora_to_egd_buf:
   buf.fields[] = decoded[]
   buf.state = BUF_VALID
   buf.last_rx_ns = clock_gettime(CLOCK_MONOTONIC)
   buf.seq_last = decoded[36].value  (sequence check)
   pthread_cond_signal(aurora_to_egd_cond)  ← T2 को bell
   Mutex unlock

6. CRITICAL: write(uio_fd, &one_u32, 4)  ← UIO re-arm
   (यह line miss हो गई तो अगली train पर interrupt NEVER आएगी!)

7. goto step 1 → poll() में block
```

**Common mistakes जो last time हुए:**
- `loopback[2:0]` port hidden था → aurora IP को re-customize करना था (M2-A Gap 10.7c)
- refclk mismatch — 125 MHz vs 156.25 MHz (M2-A Gap 10.8, solution in TCL script)
- UIO re-arm miss → second interrupt never fires

---

## 📦 Part 5: T2 Worker — Radio Announcer का काम (EGD TX)

```
1. T2 starts → pthread_cond_wait(aurora_to_egd_cond)   ← BLOCKED

2. T1 ने bell बजाई / timer expired:

3. Mutex lock aurora_to_egd_buf:
   snapshot = copy of buf.fields[] + buf.state
   Mutex unlock   ← FAST — lock कम time के लिए

4. State check:
   ├── BUF_EMPTY   → skip, do NOT publish, log once
   ├── BUF_VALID   → publish with full values
   ├── BUF_STALE   → publish with EGD Status = EGD_MESSAGE_UNHEALTHY (= 1)
   └── BUF_INVALID → skip, log error

5. JSON string बनाओ:
   {
     "i_high": 1250.5,
     "vhigh_B": 380000.0,
     "v_dcc": 820000.0,
     ...
   }
   (field names YAML से, values snapshot से)

6. JsonClient::Publish(egd_host, 18246, page_info, json_str)
   → sendto() UDP socket

7. goto step 1 → wait on condvar
```

**Note:** T2 lock जल्दी छोड़ता है — snapshot लेने के बाद। JSON build और JsonClient::Publish() **lock-free** होती हैं। इसीलिए T1 slow नहीं होता।

---

## 📦 Part 6: T3 Worker — Radio Listener का काम (EGD RX)

T3 libEGD manage करता है — तुम्हें यह thread manually create नहीं करना।

```
// startup पर:
JsonClient::Subscribe(egd_host, 18246, consumed_page_info,
                      on_egd_rx_callback, &egd_to_aurora_buf)
// libEGD automatically:
//   socket() → bind(port=18246) → recvfrom() loop → background thread
```

**on_egd_rx_callback (तुम्हारा callback, T3 context में):**
```
void on_egd_rx_callback(page_info, header, status, json_str, user_data) {
    // RULE: यहाँ BLOCK मत करो!
    
    // JSON parse करो → values निकालो
    // egd_to_aurora_buf में लिखो (mutex)
    // T4 को bell बजाओ (condvar signal)
    // RETURN IMMEDIATELY
}
```

---

## 📦 Part 7: T4 Worker — Wagon Packer का काम (Aurora TX)

```
1. T4 starts → pthread_cond_wait(egd_to_aurora_cond)   ← BLOCKED

2. T3 ने bell बजाई (new EGD command आया):

3. Mutex lock egd_to_aurora_buf:
   tx_snapshot = copy of fields[23]
   Mutex unlock

4. Pack aurora_tx_field_t[] array:
   tx[0]  = phaseA_duty  (float64 → wire float32)
   tx[2]  = packed_bits  (u32, from bit_pack fields)
   tx[10] = next_state   (s32)
   ... etc (per YAML tx.fields schema)

5. aurora_adapter_send_tx(adapter, tx, 23)
   → write 23 words to TDFD register
   → set TLR = 23 × 4 = 92 bytes
   → Aurora TX FIFO fills → AXI-Stream → Aurora PL → fiber → RTDS

6. goto step 1 → wait on condvar
```

---

## 📦 Part 8: T5 Worker — Station Master का काम (Supervisor)

```
Loop every 100 ms:
   
   // Freshness check:
   now_ns = clock_gettime(CLOCK_MONOTONIC)
   
   if (now_ns - aurora_to_egd_buf.last_rx_ns) > 500 ms:
       aurora_to_egd_buf.state = BUF_STALE
       LOG WARNING: "No Aurora frame for 500ms"
   
   if (now_ns - egd_to_aurora_buf.last_rx_ns) > 2000 ms:
       egd_to_aurora_buf.state = BUF_STALE
       LOG WARNING: "No EGD RX for 2s"
   
   // Stats log (हर 10वें wakeup पर):
   aurora_adapter_get_stats() → print:
     rx_frames_ok, rx_frames_length_mismatch, rx_frames_nan,
     rx_irq_overrun, tx_frames_sent, last_rx_inter_arrival_ns
   
   // Health file update (optional):
   write "/tmp/bridge_health": "state=ACTIVE,rx_ok=12345,stale=0"
```

---

## 📦 Part 9: Startup Sequence — एक सही order में

```
Step 1: bridge.yaml load करो
   ├── egd.producer_host, aurora.uio_device, thresholds, paths
   └── error → log + exit(1)

Step 2: EGD config XML fetch करो (HTTP GET, port 7937)
   ├── max 5 retries: 1s, 2s, 4s, 8s, 16s back-off
   ├── success → disk cache करो, var_map build करो
   └── all failed → 
         ├── cache file है और 24h से fresh है → WARNING + use cache
         └── cache नहीं / stale → log FAULT + exit(1)
   ← "Fail-closed: partial field map = silent data corruption"

Step 3: var_map validate करो
   ├── field names YAML से match होते हैं?
   ├── word counts 37/23 हैं?
   └── mismatch → exit(1)

Step 4: aurora_adapter_create(yaml_path)
   └── AURORA_ERR_CONFIG_* → exit(1)

Step 5: Buffers init करो (mutexes + condvars)

Step 6: aurora_adapter_start(uio_path, on_frame_cb, buf_ptr)
   → T1 starts, waits for first RC IRQ
   └── AURORA_ERR_UIO_OPEN → exit(1)

Step 7: JsonClient::Subscribe() → T3 starts

Step 8: pthread_create T2 → immediately blocks on condvar

Step 9: pthread_create T4 → immediately blocks on condvar

Step 10: pthread_create T5 → enters sleep loop

Step 11: main thread: pthread_join(T2) ← process alive रहता है
         SIGTERM आए → stop_flag set → सब threads clean exit
```

**महत्वपूर्ण:** Hardware I/O (Step 6) तब तक नहीं होता जब तक config valid नहीं है। Config fetch fail होने पर hardware को touch नहीं किया।

---

## 📦 Part 10: EGD Config के 6 Decisions (M2-B §9 का जवाब)

M2-B ने 6 open questions M2-D को दिए थे। अब सब decide हो गए:

| # | Question | Decision |
|---|---|---|
| 9.1 | Config server address कहाँ से? | **`bridge.yaml` config file** — `egd.producer_host` key। Hard-code नहीं। |
| 9.2 | Fetch fail तो? | **Exponential back-off 5 retries** (1s, 2s, 4s, 8s, 16s), then FAULT। **Fail-closed.** |
| 9.3 | Startup order? | EGD fetch → validate → aurora_create → buffers → aurora_start → subscribe → T2 → T4 → T5 |
| 9.4 | ProducerID कैसे? | **Explicit in config**: `egd.producer_ip` — auto-detect नहीं (multi-NIC ambiguity) |
| 9.5 | Both directions? | **Yes, bidirectional default** — `ProducedData` + `ConsumedData`। `subscribe_consumed: false` से disable हो सकता है। |
| 9.6 | Config cache? | **Disk cache** `bridge_cache/egd_config.xml`। 24h तक use करो अगर server briefly down हो। 24h बाद stale → fail-closed। |

---

## 📦 Part 11: Failure Modes — क्या हो सकता है और Bridge क्या करेगा

| Failure | Bridge का reaction | Train analogy |
|---|---|---|
| Aurora link down (channel_up = 0) | 500ms बाद BUF_STALE → T2 UNHEALTHY EGD publish करता है | Track टूट गई — आखिरी known data use करो, Radio पर UNHEALTHY announce करो |
| Frame length wrong (≠ 37 words) | BUF_INVALID → T2 publish skip | Broken train — wagons count wrong, discard, wait for next |
| Float field NaN | `is_nan=1` flag, field JSON से omit | Wagon का meter broken — उस field को report मत करो |
| EGD network down | `sendto()` error, log, continue | Radio tower temporarily down — keep trying |
| EGD RX absent | 2s बाद STALE → T4 last-known values से TX | EMS से कोई command नहीं आया — last command continue करो |
| UIO device disappears | T1 fatal error → stop_flag → clean shutdown → exit | Track ही disappear हो गई — bridge बंद करो, operator restart करे |

---

## 📦 Part 12: M3 में क्या implement करना है

| # | Task | क्या है |
|---|---|---|
| M3.1 | `aurora_adapter.c` implement करो | YAML loader, UIO open/mmap, T1 thread, TX path, stats |
| M3.2 | `canonical_buffer.c` | Buffer init/destroy, state transitions |
| M3.3 | `on_frame` callback | T1 → aurora_to_egd_buf write + T2 signal |
| M3.4 | `egd_tx.c` — T2 thread | condvar wait, JSON build, JsonClient::Publish |
| M3.5 | `egd_config_fetch.c` | HTTP GET, retries, disk cache |
| M3.6 | `egd_rx.c` — T3 callback | EGD RX → egd_to_aurora_buf write + T4 signal |
| M3.7 | `aurora_tx.c` — T4 thread | condvar wait, TX pack, aurora_adapter_send_tx |
| M3.8 | `supervisor.c` — T5 thread | freshness check, stats log, health file |
| M3.9 | `bridge_main.c` | `main()`, bridge.yaml load, startup sequence, SIGTERM handler |
| M3.10 | `bridge/CMakeLists.txt` | ARM64 build, link against libEGD.so |
| M3.11 | Unit tests | Host-side tests for buffer state, T1/T2/T3/T4 logic |
| M3.12 | Integration test on A53 | Board + bitstream + tcpdump verify |

---

## 🧠 Summary — 8 Lines में याद करो

1. **Bridge = Border Station Translator** — Aurora trains ↔ EGD radio के बीच में।
2. **5 threads:** T1 (Aurora RX), T2 (EGD TX), T3 (EGD RX, libEGD), T4 (Aurora TX), T5 (Supervisor).
3. **2 notebooks (canonical buffers):** `aurora_to_egd_buf` (37 fields) + `egd_to_aurora_buf` (23 fields). Mutex protected.
4. **Buffer states:** EMPTY → VALID → STALE → VALID cycle। INVALID = bad frame।
5. **Callbacks (T1 on_frame + T3 on_egd_rx) MUST NOT block।** बस buffer लिखो + signal करो।
6. **Startup: fail-closed।** Config wrong है तो hardware touch नहीं होता।
7. **aurora_adapter.c आज stub है।** M3 में implement होगा।
8. **Gap 10.8 (refclk) pending Vivado validation।** Software path rate-independent है।

---

## 📚 Related Documents

| Document | भाषा | क्या है |
|---|---|---|
| `M2-D_application_design.md` | English | इस doc का English mirror (reviewer के लिए) |
| `M2-A_auroralink_findings.md` + `_hindi.md` | EN + HI | Aurora FIFO registers, IRQ, UIO re-arm |
| `M2-B_libegd_findings.md` + `_hindi.md` | EN + HI | EGD API, config XML, JsonClient |
| `M2-C_cross_compile.md` + `_hindi.md` | EN + HI | libEGD ARM64 build |
| `../BRIDGE_SOFTWARE_DESIGN.md` + `_hindi.md` | EN + HI | Bridge architecture framing |
| `../DESIGN_INTENT.md` | English | EMPTY/VALID/STALE/INVALID state model, design principles |
| `../../bridge/include/aurora_adapter.h` | C header | Aurora adapter interface contract |
| `../../config/aurora_fieldmap.example.yaml` | YAML | 37 RX + 23 TX field definitions |
