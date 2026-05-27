# Bridge Software Design — पूरी समझ (Hindi में)

### 

> **यह document `BRIDGE_SOFTWARE_DESIGN.md` का Hindi mirror है।**  
> English doc reviewer / customer के लिए है। यह Hindi doc तुम्हारे खुद के लिए है — पूरा bridge architecture train metaphor के साथ।

---

## 🚉 पहले एक बड़ी कहानी — "Translator at the Border Station"

> कल्पना करो: दो देशों के बीच एक **Border Railway Station** है।
>
> - **एक side पर:** Train system है (Aurora). Trains बहुत fast आती हैं — हर 125 microseconds में एक train (एक packet), 37 wagons (37 words) हर train में, fiber-optic tracks पर।
> - **दूसरी side पर:** Radio system है (EGD). Announcements बहुत slower हैं — हर 10 ms में एक announcement, multiple platforms (subscribers) पर एक साथ broadcast।
>
> Border Station पर एक **Translator बैठा है। उसका काम है:**
>
> 1. Train आते ही wagons को decode करना (RTDS से data जो आया)
> 2. उस data को एक common notebook में लिखना (canonical model)
> 3. Radio system को उसकी language में बताना (EGD broadcast)
> 4. Reverse direction में भी — radio sunna, notebook में लिखना, अगली train के लिए wagons pack करना (EGD → Aurora TX)
>
> यह translator ही हमारा **Bridge** है। Bridge वो software है जो Kria board के ARM processor (PS) पर चलेगा।
>
> **Bridge तीन हिस्सों में बँटा है:**
> - **Aurora Adapter** — train wagons पढ़ने/भरने वाला
> - **Translation Core** — common notebook (canonical model) और translation logic
> - **EGD Adapter** — radio पर बोलने/सुनने वाला

---

## 📦 Part 1: Scope और Boundaries — किस Layer का काम कौन करता है?

Bridge एक बड़ी system का हिस्सा है। पूरी system 5 layers में बँटी है:

| Layer | कहाँ चलता है | काम क्या है | किसने बनाया |
|---|---|---|---|
| **PL — AuroraLink IP** | FPGA fabric | Fiber पर high-speed serial link, framing, AXI-Stream output | Xilinx IP (`aurora_8b10b`) |
| **PL — Aurora-to-PS bridge** | FPGA fabric | AXI-Stream को memory-mapped FIFO में convert | Xilinx IP (`axi_fifo_mm_s`) |
| **PS — Aurora ingest/egress** | ARM A53 (Ubuntu Linux) | FIFO से words pull करना, IRQ handle, words को application structs में unpack | M2-D scope (हम implement करेंगे) |
| **PS — Translation / Mediation** | ARM A53 | **Bridge का core** — state, buffering, mapping, mode switching | M2-D scope (हम implement करेंगे) |
| **PS — EGD I/O** | ARM A53 | UDP send/receive, EGD page encode/decode | `libEGD` (`JsonClient`) |

**इस document का focus:** ऊपर के table में **bold/italic** layers — Aurora ingest+egress, Translation, EGD I/O। ये तीनों PS पर run होते हैं और मिलकर "bridge software" बनाते हैं।

**Train analogy summary:**

```
RTDS (दूसरा देश)         FPGA Fabric (Border पर hardware)        ARM Processor (Border पर software)
                                                                         │
[Trains आती हैं]   ──►   [AuroraLink IP]      [AXI FIFO]      ──►   [Bridge Software]
                                                                    ├── Aurora Adapter
                                                                    ├── Translation Core
                                                                    └── EGD Adapter
                                                                         │
                                                                         ▼
                                                                   Ethernet/UDP
                                                                         │
                                                                         ▼
                                                                  EGD Subscribers
                                                                  (दूसरा देश की radios)
```

---

## 📦 Part 2: Aurora Side — Data Processor तक कैसे पहुँचता है?

### 2.1 Physical Path

```
RTDS (remote)  ──fiber──►  [Aurora IP in PL]
                                  │
                             AXI-Stream
                                  │
                           [FIFO bridge IP]
                                  │
                           AXI4 (memory-mapped)
                                  │
                           PS (ARM Cortex-A53)
                                  │
                           Bridge Software (हम)
```

**Critical समझ:** **AuroraLink FPGA में चलता है, ARM cores में नहीं।** ARM software को serial link को implement नहीं करना। ARM software को बस एक memory-mapped peripheral (AXI FIFO) से read/write करना है।

> **Train analogy:** Border Station पर तुम (translator) train tracks नहीं चलाते। तुम्हारे सामने एक **conveyor belt (FIFO)** है — train के wagons उसी पर आते हैं। तुम्हें बस conveyor belt से wagons उठाने हैं।

### 2.2 Aurora Adapter — Reference Implementation

Existing PEBB CHIL reference (`Axi_IO.c`) यह करता है:

| Function | काम क्या है | Train language में |
|---|---|---|
| `rxData[]`, `txData[]` tables | हर 32-bit word का meaning declare करना (नाम, type, scaling) | "Wagon 1 में voltage A है, wagon 2 में voltage B है, … wagon 37 में status flag है" |
| `axiReceive()` | RX FIFO को word-by-word drain करना, values controller variables में लिखना | "Conveyor belt से सब wagons उठाओ, हर wagon का content notebook में लिखो" |
| `axiSend()` | Application state को pack करके TX FIFO भरना | "Notebook से values उठाओ, wagons बनाकर outgoing conveyor belt पर रख दो" |

**हमारे bridge के लिए M2-D में यह काम fresh करना है** — Aurora adapter को re-implement करना है **config-driven** approach में (hard-coded `rxData[]` table नहीं, YAML field-map से load करो)। Architecture identical है; flexibility नई है।

### 2.3 Push vs Pull — सबसे confusing सवाल

"Push" और "pull" शब्दों में confusion आ जाता है। तो clearly लिखते हैं:

| Step | कौन करता है | Type | Train analogy |
|---|---|---|---|
| **1. Aurora → FIFO** | PL (hardware) | **Push** | Train आ रही है, conveyor belt पर wagons अपने आप गिर रहे हैं। तुम कुछ नहीं कर रहे। |
| **2. FIFO → PS** notification | PL (hardware) | **Hardware interrupt** | Belt पर 37 wagons पूरे हो गए — एक **bell बज जाती है** (`pl_ps_irq0`)। |
| **3. FIFO → application** | PS (CPU) | **Pull** | Bell सुनकर तुम (CPU) belt पर जाते हो, हर wagon को **खुद उठाते हो** (`XLlFifo_RxGetWord()`)। |

**मतलब:**
- Aurora → FIFO हिस्सा **automatic** है, hardware level पर।
- FIFO → CPU हिस्सा **interrupt-driven notification + processor-initiated pull** है।

CPU सोता रहता है जब तक bell नहीं बजती। जैसे ही bell बजी, CPU जागकर सब wagons पढ़ता है।

### 2.4 ये 4 चीज़ें M2-D शुरू करने से पहले verify करनी हैं:

| # | सवाल | क्यों matter करता है |
|---|---|---|
| 1 | RX हमेशा interrupt-driven है, या polled / DMA भी हो सकता है? | अगर polled है तो CPU हमेशा busy रहेगा (latency बेहतर पर power waste) |
| 2 | Hardware "frame complete" expose करता है, या सिर्फ "FIFO non-empty"? | अगर सिर्फ non-empty तो software को खुद decide करना है कि packet complete हुआ कि नहीं — TLAST का use करना पड़ेगा |
| 3 | FIFO length register की semantics — partial read, error recovery? | `ReceiveLength` vs `RECEIVE_LENGTH` pattern from reference code |
| 4 | Cache coherency — कोई buffer DDR में है जो PL देख सकता है? | अगर हाँ — `non-cacheable` mappings चाहिए या explicit flush/invalidate |

M2-A में सब verify हो गया है — सब points के answers `M2-A_auroralink_findings.md` § 3, 4, 11.4 में हैं। हमारे case में:
- **Answer 1:** Interrupt-driven (`pl_ps_irq0` → UIO)
- **Answer 2:** Aurora "Framing mode" में `tlast` assert करता है; FIFO उसी पर RC IRQ देता है — मतलब frame-complete level पर notification मिलता है
- **Answer 3:** Reference uses `XLlFifo_iRxOccupancy()` + `XLlFifo_iRxGetLen()`; M2-D adapter वही pattern follow करेगा
- **Answer 4:** FIFO MMR (`mmap` of `/dev/uio0`) is uncached by default; कोई DMA नहीं — कोई coherency issue नहीं

---

## 📦 Part 3: EGD Side — libEGD का काम

EGD interface पूरी तरह software में implement है — `libEGD` C++ library।

### Bridge को क्या use करना है:

| API | काम | जब use करना है |
|---|---|---|
| `JsonClient::Publish()` | EGD message भेजना | Aurora-side से नया data आया, EGD subscribers को बताना है |
| `JsonClient::Subscribe()` | EGD message receive करना | EMS से commands सुनना है (बाद में Aurora TX में बदल देंगे) |

### `JsonClient` क्यों, `EgdClient` क्यों नहीं:

Detailed comparison `M2-B_libegd_findings_hindi.md` Part 7 में है। Short answer: `JsonClient` use करो — automatic encoding, naming by JSON keys, endianness internally handle। Performance overhead negligible है हमारे rates पर।

### Critical rule:

Bridge को **NEVER** Aurora framing को libEGD के अंदर embed नहीं करना। मतलब:

❌ **Wrong:** Aurora से raw 148 bytes (37 words × 4 bytes) उठाओ → libEGD में direct send → "EGD message का payload Aurora packet होगा"

✅ **Right:** Aurora से 37 words उठाओ → Aurora adapter unpack करे → canonical model में लिखो → EGD adapter उसे EGD page layout में pack करके send करे

**क्यों?** क्योंकि Aurora और EGD दो अलग protocols हैं अलग-अलग field layouts के साथ। Customer का EGD config XML बताता है कि EGD page में कौन सी field कहाँ है। Aurora's `Axi_IO.h` table बताता है कि Aurora packet में कौन सा word क्या है। दोनों के बीच एक **translation step** ज़रूरी है।

> **Train analogy:** अगर तुम train wagons को सीधे radio पर बोलो — "wagon 1, wagon 2, ..." — तो radio listener को कुछ समझ नहीं आएगा। तुम्हें पहले wagons को **values** में decode करना है (voltage = 230.5 V), फिर उन values को radio's **format** में बोलना है ("VOLTAGE_PHASE_A = 230.5")।

---

## 📦 Part 4: Mediation Layer — Bridge का दिल

यह **नया software** है जो हम M2-D में लिखेंगे। यह `Axi_IO`-style I/O और `libEGD` I/O के बीच का "bounded context" है।

### 4.1 Bridge की 6 जिम्मेदारियाँ:

| Function | काम | Train analogy |
|---|---|---|
| **Ingress adapters** | Aurora-side words को internal canonical model में normalize करो | Wagons को unpack करो, values को notebook में लिखो |
| **Egress adapters** | Canonical state को EGD page और Aurora TX layout में map करो | Notebook से radio announcement और reverse train के wagons बनाओ |
| **State machine** | Link health, modes, startup, fault, safe outputs — सब states को handle करो | Train cancel हो गई? Track damage हुआ? Translator decide करे क्या करना है |
| **Buffers** | Latest-value buffers per field, timestamps, validity flags | Notebook में हर field के साथ "last update time" और "valid / stale / invalid" stamp |
| **Scheduling** | Aurora's event-driven activity को EGD's cyclic publication से reconcile करो | Trains async आती हैं, radio announcements periodic होती हैं — दोनों को sync करो |
| **Observability** | Drops, length mismatches, sequence gaps, NaN/glitch counters | Translator हर error को log करे — कितनी bad trains आईं, कितने wagons miss हुए |

### 4.2 Module Boundaries (logical structure):

```
            ┌───────────────────────────┐
            │   Aurora Adapter           │  ◄── XLlFifo driver, UIO
            │   - RX unpack (words → values)
            │   - TX pack (values → words)
            └─────────────┬─────────────┘
                          │
            ┌─────────────▼─────────────┐
            │   Translation Core         │
            │   - State machine          │
            │   - Canonical model        │  ◄── shared latest-value buffer
            │   - Mapping tables         │     with timestamps + validity
            │   - Field-map config       │
            └─────────────┬─────────────┘
                          │
            ┌─────────────▼─────────────┐
            │   EGD Adapter              │  ◄── libEGD (JsonClient)
            │   - Publish (canonical → EGD page)
            │   - Subscribe (EGD page → canonical)
            └───────────────────────────┘
```

**Important:** "Threads vs single loop" यह implementation detail है। ऊपर का **logical** separation बना रहना चाहिए चाहे तुम 1 thread use करो या 5।

### 4.3 Existing CHIL Code के साथ Interaction:

दो options हैं:

| Option | क्या करना है | Pros | Cons |
|---|---|---|---|
| **A. Evolve in place** | Existing `Axi_IO` + main loop को extend करो, mediation + libEGD call करो | Fastest bring-up | Higher coupling (Aurora और EGD logic एक ही file में मिल जाएगा) |
| **B. Sidecar process** | Separate process, shared memory या message queue से communicate | Cleaner isolation, easier testing | More integration work |

**Recommendation:** Option B (sidecar) — बेहतर है long-term, क्योंकि bridge और existing CHIL code अलग deployable units होंगे। M2-D में formal decision record करेंगे।

---

## 📦 Part 5: Threading और Synchronization — सबसे tricky हिस्सा

### Decisions जो M2-D को लेनी हैं:

| Concern | Option 1 | Option 2 |
|---|---|---|
| **FIFO RX → translation** | Same thread as ISR — minimum latency पर stack risk + ISR में heavy work | ISR worker queue पर defer करे — recommended for heavy translation |
| **Translation → EGD TX** | Timer-driven publish (हर 10 ms पर) | "Dirty flag" approach — हर Aurora update के बाद immediate publish |
| **EGD RX → Aurora TX** | Dedicated reader thread per (host, port) (libEGD already does this) | Single shared reader thread |
| **Locking** | Mutex around canonical buffer — simple, slight contention | Lock-free single-writer / single-reader with aligned words — fast but error-prone |

### Recommended starting point (simplest correct):

```
Thread 1: Aurora RX worker
   Loop:
     poll(/dev/uio0)
     for each word in FIFO:
        canonical_buffer.set(field_name, value, timestamp)  ◄── mutex
     write(uio_fd, &one, 4)  // re-arm UIO

Thread 2: EGD TX scheduler (cyclic)
   Loop:
     sleep until next 10 ms tick
     snapshot = canonical_buffer.snapshot()  ◄── mutex
     JsonClient::Publish(snapshot.as_json())

Thread 3: EGD RX (libEGD spawns this automatically)
   Callback:
     canonical_buffer.set(field, value, timestamp)  ◄── mutex
     return immediately (no heavy work)

Thread 4: Aurora TX worker (when bidirectional)
   Loop:
     wait on canonical_buffer.tx_changed event
     pack words from buffer
     write to TX FIFO
```

> **Train analogy:** चार workers हैं — एक wagons receive करता है, एक radio पर बोलता है, एक radio से सुनता है, एक wagons भेजता है। सब एक common notebook (canonical buffer) से लिखते-पढ़ते हैं। Notebook access पर एक "तालाबंदी" (mutex) है ताकि एक साथ दो लोग एक page पर न लिखें।

### Anti-patterns जो avoid करने हैं:

❌ **EGD subscriber callback में heavy work** — libEGD's internal thread block हो जाएगा, अगला message miss हो सकता है। बस buffer में लिखो और return करो।

❌ **Aurora RX ISR में EGD publish** — IRQ handler 8 kHz पर fire होगा; उसमें EGD send कर के 10 ms wait करना disaster है।

❌ **No backpressure** — अगर canonical buffer full है तो उसको silently drop न करो, log करो।

❌ **Per-field individual mutexes** — deadlock + complexity। एक buffer-level lock से शुरू करो; profile करके optimize करो।

---

## 📦 Part 6: Verification Hooks — कैसे Test करेंगे

### Lab Test Plan:

| Layer | Test | क्या verify होता है |
|---|---|---|
| **Aurora path** | Length mismatch + sequence jump inject करो (पैटर्न generator से) | `INVALID` flag set होता है? Safe output mode में जाते हैं? |
| **EGD path** | Second host से known Producer/Exchange ID भेजो; tcpdump (PCAP) से wire compare करो | EGD message correctly format में जा रहा है? |
| **End-to-end latency** | Aurora event timestamp → EGD wire timestamp distribution | p50, p99, max latency आपके budget में हैं? |

### Specific test commands जो हम implement करेंगे:

1. **Aurora RX loopback** — patterns generator → FIFO → bridge → canonical buffer → assert per-field values match expected.
2. **EGD Publish** — bridge publishes → second machine pe `tcpdump -i eth0 udp port 18246` → PCAP को decode करो → assert EGD message structure सही है, field values match buffer।
3. **EGD Subscribe + Aurora TX** (when implemented) — second machine से EGD broadcast → bridge receives → canonical buffer update → Aurora TX FIFO में data → loopback से RX FIFO में आए → assert match।

### Failure injection tests:

| Fault | कैसे inject करो | Bridge को क्या करना चाहिए |
|---|---|---|
| EGD config server down at startup | Block port 7937 in iptables | Bridge fail-closed (logged error, exit) OR retry-forever — M2-D decide करे |
| Aurora link down mid-operation (`channel_up = 0`) | Disconnect fiber / pull SFP+ | Bridge को `STALE` mark करना चाहिए, EGD publish बंद नहीं करना — सिर्फ stale data भेजे |
| EGD subscriber crash | Stop subscriber process | Bridge को कुछ नहीं फर्क पड़ेगा (UDP fire-and-forget); लेकिन अगर bidirectional है तो EMS commands बंद हो जाएँगे |
| Slow EGD RX callback | callback में `sleep(1)` inject करो | libEGD थread block — agla message miss हो सकता है। Bridge को defensive होना चाहिए। |

---

## 🧠 Summary — 7 Lines में याद करो

1. **Bridge = Border-Station Translator।** Aurora (trains) ↔ EGD (radio) के बीच में बैठा एक software piece।
2. **तीन हिस्से:** Aurora Adapter (wagons read/write), Translation Core (canonical notebook), EGD Adapter (radio talk/listen)।
3. **Aurora RX path:** PL push → FIFO IRQ → CPU pull। CPU सोता है जब तक IRQ नहीं आता।
4. **EGD path:** UDP broadcast। 1 producer → N subscribers। `JsonClient` use करो।
5. **Bridge को Aurora framing को EGD में embed कभी नहीं करना।** हमेशा canonical model से translate करो।
6. **Threading:** 4 threads minimum — Aurora RX worker, EGD TX scheduler, EGD RX (libEGD), Aurora TX worker। एक common buffer mutex से protected।
7. **Verification:** lab में 3 levels पर test — Aurora alone, EGD alone, end-to-end latency।

---

## 🎯 अगला कदम — M2-D Architecture में क्या है

M2-D bridge implementation का formal design करेगा:

1. Configuration loading (YAML field map से)
2. Canonical buffer का exact data structure
3. Thread model का final decision (Option A vs B)
4. Aurora Adapter का skeleton → full implementation
5. EGD Adapter wrapper around `JsonClient` (with retry + reconnect)
6. State machine: `STARTUP → ACTIVE → DEGRADED → FAULT`
7. Lab test plan execution (above section 6 के tests)

Open items जो already documented हैं:
- `M2-A_auroralink_findings.md` Gap 10.5 — `aurora_adapter.c` stub है, M2-D में implement होगा
- `M2-B_libegd_findings.md` § 9 — EGD config XML fetch की 6 decisions M2-D को लेनी हैं

---

## 📚 Related Documents

| Document | भाषा | क्या है |
|---|---|---|
| `BRIDGE_SOFTWARE_DESIGN.md` | English | इस doc का English mirror (reviewer/customer के लिए) |
| `milestone2/M2-A_auroralink_findings.md` + `_hindi.md` | EN + HI | Aurora-side detail (protocol, IP, FIFO, IRQ) |
| `milestone2/M2-B_libegd_findings.md` + `_hindi.md` | EN + HI | EGD-side detail (protocol, API, config XML) |
| `milestone2/M2-C_cross_compile.md` + `_hindi.md` | EN + HI | libEGD को ARM64 के लिए cross-compile करने की pipeline |
| `milestone2/M2-D_TASK.md` | English | M2-D का task spec |
| `explaination_docs/detailed_explaination.md` + `_hindi.md` | EN + HI | RTDS-side context + Aurora deep dive |
| `explaination_docs/KR260_auroralink.md` + `_hindi.md` | EN + HI | KR260 board पर setup guide |
