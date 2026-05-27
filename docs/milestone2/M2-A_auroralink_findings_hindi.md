# 🔬 M2-A Deep Dive — हर सवाल का जवाब (Hindi में)
## RTDS → Aurora → AXI Stream → FIFO → Interrupt — पूरी Journey

---

## ⚠ पहले यह पढ़ो — RTDS Hardware नहीं है तो क्या करें?

इस document में हर जगह लिखा है "RTDS यह भेजता है, RTDS वो भेजता है"। **लेकिन हमारे पास RTDS hardware नहीं है।** तो test कैसे करेंगे?

जवाब: हम **5 stages** में verify करते हैं (full detail: `docs/milestone2/M2-A_auroralink_findings.md` §12 और `explaination_docs/KR260_auroralink.md` §8):

| Stage | क्या करना है | Hardware चाहिए? | क्या prove होता है |
|---|---|---|---|
| **S1** | Vivado में RTL simulation — PG046 का built-in testbench चलाओ। दो Aurora IPs back-to-back simulate होते हैं। | सिर्फ PC | Aurora IP correctly configured है (framing, clocks, TLAST, सब सही हैं) |
| **S2** | Kria board पर: Pattern Generator → AXI FIFO → IRQ → PS Software। **Aurora bypass** — Aurora को skip करो। | एक Kria | FIFO + IRQ + Linux UIO + libEGD + software path सब correct है, Aurora पर depend नहीं |
| **S3** | वही S2 लेकिन अब Pattern Generator → TX FIFO → Aurora TX → **PMA Loopback** (TX को internally RX से जोड़ा) → Aurora RX → RX FIFO → IRQ → PS | एक Kria + **Aurora IP को re-customise करना पड़ेगा** (नीचे देखो) | Aurora actually काम करता है — `channel_up=1`, refclk सही है, SERDES में data round-trip सही होता है |
| **S4** (optional) | SFP+ optical module + 1 m LC-LC fiber TX से RX में लगाओ (same cage पर) | Kria + ~£50 का hardware | Physical optical layer भी काम करता है |
| **S5** | Real RTDS से fiber connect करो | RTDS hardware | Production interop |

**हमारा demo S1 + S2 + S3 तक करेंगे।** S4 optional है, S5 RTDS आने तक defer।

### ⚠ ⚠ बहुत Important — S3 की वो गलती जो last time समय बर्बाद कर गई

`top.xsa` में जो Aurora IP supplied है उसमें **`loopback[2:0]` pin top-level पर expose नहीं है** — IP wrapper ने उसे internally `3'b000` (normal mode) पर tie कर रखा है। मतलब:

- अगर तुम supplied bitstream load करके xlconstant से `loopback = 3'b010` set करोगे → **कुछ नहीं होगा।** IP वो pin देखता ही नहीं।
- `channel_up` कभी `1` नहीं होगा (क्योंकि actual loopback hi नहीं हो रही)।
- तुम सोचोगे software में बग है, घंटों डिबग करोगे, जबकि असली problem hardware-side IP customisation है।

**Fix:** Vivado में Aurora IP खोलो → "Show optional ports" / "Debug and Control" tab में जाओ → **`loopback` port को tick करो** → IP regenerate करो → फिर bitstream बनाओ। हमारा `scripts/vivado/build_aurora_loopback.tcl` यह assume करता है कि तुमने यह step पहले कर लिया है।

### S3 की दूसरी चीज़ — Refclk Frequency (Gap 10.8 → ✅ SOLVED)

Stock KR260 starter kit के carrier card पर oscillator **156.25 MHz** है (per XTP743 page 16) — 10G Ethernet के लिए। हमारा Aurora IP `design_1.hwh` में 125 MHz expect करता है। पहले यह problem लग रहा था। **अब solve हो गया।**

**Solution: Lab-Variant Aurora IP rebuild** —

| Setting | Production target | Lab variant (stock KR260) |
|---|---|---|
| `C_REFCLK_FREQUENCY` | 125 MHz | **156.25 MHz** |
| `C_LINE_RATE` | 2 Gbps (= 125 × 16) | **2.5 Gbps (= 156.25 × 16)** |
| GTH PLL multiplier | ×16 | **×16 — same integer** |
| Aurora user_clk_out | 50 MHz | 62.5 MHz |
| Software path | identical | **identical** |
| FIFO, IRQ, framing, TLAST | identical | **identical** |

**मतलब:** Lab variant पर सब verify हो जाता है — software, FIFO, IRQ, libEGD, translation layer। Production में जब custom carrier या oscillator swap हो जाएगा, बस 2 parameter बदलने हैं (`C_REFCLK_FREQUENCY` और `C_LINE_RATE`), aur कुछ नहीं।

**Build command:**
```bash
# Stock KR260 lab variant (default — कुछ argument नहीं देना)
vivado -mode batch -source scripts/vivado/build_aurora_loopback.tcl

# Production target (बाद में जब custom carrier मिले)
vivado -mode batch -source scripts/vivado/build_aurora_loopback.tcl \
       -tclargs --refclk_mhz 125 --line_rate_gbps 2
```

Senior को यह बोलो: *"Gap 10.8 is resolved via a lab-variant Aurora IP rebuild at 156.25 MHz / 2.5 Gbps. Production switch is a two-parameter change once the right hardware is available."* — यह "defer" नहीं, "solved by parametric rebuild" है।

---

## 🎯 तुम्हारे सवाल (एक-एक करके जवाब दूँगा):

1. क्या RTDS सारा data एक साथ भेजता है? (जैसे एक file?)
2. Aurora hardware कितना data एक साथ AXI stream में convert करता है?
3. AXI Stream क्या है?
4. FIFO कैसा होता है — queue में एक जगह कितना data?
5. Interrupt signal कब भेजता है?
6. "Complete Packet" क्या होता है?

> **Note:** नीचे जहाँ भी "RTDS भेजता है" लिखा है, उसे पढ़ो as: **हमारे लाब में** S2 stage में Pattern Generator भेजता है, S3 में Pattern Generator → Aurora TX → loopback → Aurora RX भेजता है, और production में real RTDS भेजता है। Behaviour सब cases में same है क्योंकि frame format एक है।

---

# 🎭 पहले एक नई Story — "Train और Platform"

> **RTDS** = एक Train Driver जो simulation step rate पर एक **Train (packet)** भेजता है (typical assumption: ~125 µs / 8 kHz; exact value pending RTDS docs citation — see `docs/milestone2/M2-A_auroralink_findings.md` Known Gaps)।  
> यह train हमेशा **same size** की होती है — exactly **37 डिब्बे** (words)।
>
> **Aurora IP** = Railway Track + Signal System। Train को fiber optic track पर receive करता है।
>
> **AXI FIFO** = Railway Platform + Waiting Area। Train आती है, platform पर रुकती है।
>
> **Interrupt** = Platform पर लगी **घंटी** — "ट्रेन आ गई! अब passengers उतर सकते हैं!"
>
> **A53 CPU** = Passengers (data) को unload करने वाला worker।

---

# ❓ सवाल 1: क्या RTDS "सारा data एक साथ" भेजता है?

## जवाब: हाँ — RTDS एक बार में एक **पूरा packet** भेजता है।

### यह "file" नहीं है — यह "snapshot" है

RTDS एक **Real-Time Power System Simulator** है।  
यह electricity के circuits को real-time simulate करता है।

**RTDS हर simulation step पर** (model rate; observed in CHIL reference comments as "~125 µs / 8 kHz" but **not** yet sourced from an RTDS docs citation — see Known Gaps in `docs/milestone2/M2-A_auroralink_findings.md`) एक नया step calculate करता है:
- Voltage = कितना? → ek number
- Current = कितना? → ek number
- State command = क्या? → ek number
- ... और ऐसे 36 values

इन **सभी values को एक साथ pack करके** वो एक **packet** बनाता है।  
यह packet हमेशा **fixed size** का होता है — **37 × 4 bytes = 148 bytes**।

### 🚂 Train analogy:
```
RTDS हर simulation step पर (~125 µs assumed; pending RTDS docs citation) एक "Train" भेजता है
Train में हमेशा exactly 37 डिब्बे होते हैं:
  डिब्बा 1  = Key (ignore करो)
  डिब्बा 2  = i_high (current A phase) = 1.25 kA
  डिब्बा 3  = vhigh (voltage B phase) = 11.0 kV
  डिब्बा 4  = vhigh (voltage A phase) = 11.0 kV
  ...
  डिब्बा 35 = run command (0 या 1)
  डिब्बा 36 = RXI_chil flag
  डिब्बा 37 = sequence number (1-1000)
```

### Code proof — `Axi_IO.h` line 23:
```c
#define RECEIVE_LENGTH 37  // 37 words हमेशा आते हैं — fixed!
```

---

# ❓ सवाल 2: Aurora Hardware — कितना data एक साथ convert करता है?

## जवाब: Aurora **एक पूरा packet** receive करता है और **एक complete AXI transaction** के रूप में FIFO में देता है।

### Step-by-step Aurora का काम:

```
Fiber Optic Cable
     │
     │  (Serial bits — एक के बाद एक bits आते हैं)
     │  010110001011100010...
     │
     ▼
┌─────────────────────────────────┐
│   Aurora 8B/10B IP Core (PL)    │
│   `aurora_8b10b` v11.1 (PG046)  │
│   1 lane × 2 Gbps               │
│                                  │
│  1. Serial bits को decode करो   │
│  2. 8B/10B line encoding हटाओ   │
│     (per design_1.hwh — यह design  │
│      Aurora 64B/66B नहीं है)    │
│  3. Actual data words बनाओ      │
│  4. AXI4-Stream format में दो   │
└─────────────────────────────────┘
     │
     │  AXI4-Stream (parallel data — organized)
     ▼
```

### Aurora कितना data एक साथ process करता है?

Aurora IP core typically **32-bit या 64-bit wide** होता है।  
यानी हर **clock cycle** में **4 bytes (1 word) या 8 bytes (2 words)** process होते हैं।

लेकिन तुम्हारे लिए जो important है वो यह है:

> **Aurora IP का काम है बस bits को flip करना** — serial → parallel।  
> जो RTDS ने "एक packet" भेजा, वो **Aurora उसे AXI-Stream में convert करके FIFO में डाल देता है।**  
> तुम्हें individual clock cycles की चिंता नहीं करनी।

---

# ❓ सवाल 3: AXI Stream क्या है?

## जवाब: AXI-Stream एक "Highway" है जिस पर data chips के अंदर travel करता है।

### पहले — AXI क्या है?

**AXI = Advanced eXtensible Interface**  
यह Xilinx/ARM का एक **standard protocol** है।  
जैसे USB एक standard है — कोई भी USB device किसी भी computer से connect होती है।  
ऐसे ही — कोई भी AXI device किसी भी AXI bus से connect होती है।

### AXI-Stream specifically:

```
                    Data flow (एक direction में)
Aurora IP  ────────────────────────────────────►  AXI FIFO
           
           हर clock cycle में एक "beat" जाती है:
           
           Beat 1: [DATA=0x3F800000] [VALID=1] [LAST=0]  ← बीच में है
           Beat 2: [DATA=0x40000000] [VALID=1] [LAST=0]  ← बीच में है
           Beat 3: [DATA=0x3E000000] [VALID=1] [LAST=0]  ← बीच में है
           ...
           Beat 37:[DATA=0x00000064] [VALID=1] [LAST=1]  ← आखिरी! LAST=1
```

### तीन important signals:

| Signal  | मतलब                               |
| ------- | ---------------------------------- |
| `DATA`  | 32-bit actual data (1 word)        |
| `VALID` | "यह data valid है" — sender कह रहा है |
| `LAST`  | "यह आखिरी beat है" — packet खत्म!      |

### 🏎️ Highway analogy:
```
AXI-Stream = एक one-way highway

हर second एक car निकलती है (हर clock cycle एक word)
Car की number plate पर DATA लिखा है
Car की windshield पर VALID/LAST flag है

जब LAST=1 वाली car आती है → "convoy खत्म!"
                             → FIFO को interrupt signal मिलता है
```

### यह "stream" है — इसलिए:
- Data **रुकता नहीं** — continuously flow करता है
- **कोई address नहीं** — बस sequential words
- **Master → Slave** direction (Aurora = master, FIFO = slave)

---

# ❓ सवाल 4: FIFO कैसा होता है? Queue में कितना data?

## जवाब: FIFO एक "Line/Queue" है — पहले आया, पहले जाएगा।

### FIFO की Physical Structure:

```
FIFO अंदर से ऐसा दिखता है (एक पानी की टंकी की तरह):

        ┌─────────────────────────┐
        │                         │
IN ──►  │  [slot 1] word  = 0x3F  │
        │  [slot 2] word  = 0x40  │
        │  [slot 3] word  = 0x3E  │
        │  [slot 4] word  = 0x41  │
        │  [slot 5] word  = 0x00  │
        │  ...                    │
        │  [slot N] word  = ????  │  ← खाली slots
        │                         │
        └──────────┬──────────────┘
                   │
               OUT ▼
            (CPU यहाँ से पढ़ता है)
```

### FIFO की Size कितनी होती है?

**AXI4-Stream FIFO (PG080)** — इसकी depth configurable होती है।  
Common values: **512, 1024, 4096 words (32-bit each)**

हमारे case में:
- हर packet = **37 words = 148 bytes**
- FIFO depth = कम से कम **512 words** होगी
- यानी FIFO में एक बार में **~13-14 packets** आ सकते हैं store होने के लिए

> लेकिन practically — हमारा system हर packet के साथ process करता है,  
> तो FIFO में हमेशा **सिर्फ 1 packet** होता है (37 words)।

### FIFO में दो Pointer होते हैं:

```
Write Pointer (WR_PTR) ──►  Aurora यहाँ data लिखता है
Read Pointer  (RD_PTR) ──►  CPU यहाँ से data पढ़ता है

WR_PTR आगे बढ़ता है जब data आता है
RD_PTR आगे बढ़ता है जब CPU पढ़ता है

जब WR_PTR > RD_PTR → FIFO में data है (Occupancy > 0)
जब WR_PTR = RD_PTR → FIFO खाली है
```

### Code में देखो — `Axi_IO.c` line 610:
```c
if (XLlFifo_iRxOccupancy(InstancePtr)) {
//   ↑ यह check करता है: "क्या FIFO में कुछ है?"
//   अगर WR_PTR > RD_PTR → true → data है
```

### FIFO में "एक जगह" कितना data?

```
हर "slot" = 1 word = 32 bits = 4 bytes
FIFO depth = typically 512 words
Total capacity = 512 × 4 = 2048 bytes = 2 KB

हमारा 1 packet = 37 words = 148 bytes
इसलिए FIFO overflow नहीं होता।
```

---

# ❓ सवाल 5: Interrupt signal कब भेजता है? (सबसे Important!)

## जवाब: जब RTDS का **एक complete packet का आखिरी word** FIFO में आ जाता है।

### यह कैसे detect होता है?

AXI4-Stream FIFO IP core internally यह track करता है:

```
Aurora IP ──► AXI-Stream ──► FIFO IP Core
                                │
                         LAST=1 आया?
                                │
                          ┌─────▼──────┐
                          │  YES!      │
                          │  Packet    │
                          │  Complete! │
                          └─────┬──────┘
                                │
                         RC (Receive Complete)
                         interrupt flag SET होता है
                                │
                                ▼
                         A53 CPU को signal जाता है
                         "Data ready hai! Aa jao!"
```

### RTDS "LAST" signal कैसे देता है?

RTDS हमेशा **37 words** भेजता है।  
Aurora IP जानता है packet का end कब है — AXI-Stream का `TLAST` signal यही बताता है।

```
Aurora → FIFO: Beat 1  (TLAST=0) → word 1 आया
Aurora → FIFO: Beat 2  (TLAST=0) → word 2 आया
Aurora → FIFO: Beat 3  (TLAST=0) → word 3 आया
...
Aurora → FIFO: Beat 37 (TLAST=1) → word 37 आया ← PACKET COMPLETE!
                                                    INTERRUPT FIRE!
```

### Timeline देखो:

```
Time ──────────────────────────────────────────────►

RTDS भेजता है:   [pkt1 start]...[pkt1 end] [pkt2 start]...[pkt2 end]
                                  │                           │
                                  ▼                           ▼
FIFO INTERRUPT:               [INT!]                      [INT!]
                                  │                           │
A53 CPU:              [idle] ► [FifoRecvHandler]  [idle] ► [FifoRecvHandler]
                               [reads 37 words]            [reads 37 words]
                               [runs controller]           [runs controller]
                               [sends TX data]             [sends TX data]
                               [goes idle again]           [goes idle again]
```

### Code में interrupt setup — `main.c` line 281:
```c
XLlFifo_IntEnable(InstancePtr, XLLF_INT_ALL_MASK);
// ↑ यह line FIFO को enable करती है interrupt देने के लिए
// जब TLAST=1 आएगा → RC interrupt → FifoHandler call होगा
```

### Code में interrupt check — `main.c` line 434:
```c
if (Pending & XLLF_INT_RC_MASK) {
//              ↑ RC = Receive Complete
//   यह tab true होता है जब एक complete packet FIFO में आ जाता है
    FifoRecvHandler(InstancePtr);  // ← तब यह call होता है
}
```

---

# ❓ सवाल 6: "Complete Packet" क्या होता है?

## जवाब: एक Complete Packet = एक RTDS simulation step का पूरा data।

### Packet की Definition:

```
┌─────────────────────────────────────────────────────┐
│                 ONE COMPLETE PACKET                 │
│                 = 37 words = 148 bytes              │
├──────┬────────────────────────────────────────────  │
│ Word │ Content                                      │
├──────┼────────────────────────────────────────────  │
│  1   │ Key (security/sync key)                      │
│  2   │ i_high — Phase A current (float, kA)        │
│  3   │ vhigh B — Phase B voltage (float, kV)       │
│  4   │ vhigh A — Phase A voltage (float, kV)       │
│  5   │ v_dcc — DC capacitor voltage                │
│  6   │ i_low — Low side current                    │
│  7   │ (dummy — unused)                            │
│  8   │ PEBB_NEXT_STATE_CMD (float)                 │
│  9   │ P_CMD — Active power command (float, MW)    │
│  10  │ LOWSIDE_CONTACTOR_CMD                       │
│  11  │ PEBB_POWER_LOWER_LIMIT                      │
│  12  │ PEBB_POWER_UPPER_LIMIT                      │
│13-14 │ (dummies)                                   │
│  15  │ ctr — Sub-step counter (s32)               │
│16-19 │ (dummies)                                   │
│  20  │ V_CMD — Voltage command                     │
│  21  │ ForceFault command                          │
│22-25 │ (dummies)                                   │
│  26  │ EN_GRID_SUPPORT                             │
│  27  │ Q_CMD — Reactive power command (MVA)        │
│  28  │ Q_LIMIT                                     │
│  29  │ i_high_B — Phase B current                 │
│  30  │ i_high_C — Phase C current                 │
│  31  │ v_high_c — Phase C voltage                 │
│  32  │ v_stator_A                                  │
│  33  │ v_stator_B                                  │
│  34  │ v_stator_C                                  │
│  35  │ run — Run/Stop command (s32: 0 या 1)       │
│  36  │ RXI_chil — Loop closure flag               │
│  37  │ seq — Sequence number (1 to 1000)          │
└──────────────────────────────────────────────────── ┘
```

### "Complete" का मतलब:

**Incomplete packet (गलत) :**
```
RTDS ने word 1-20 भेजे → connection टूट गई → word 21-37 नहीं आए
FIFO में सिर्फ 20 words हैं
यह "incomplete packet" है → DISCARD करो!
```

**Complete packet (सही) :**
```
RTDS ने word 1-37 भेजे → सभी आए → TLAST=1 मिला
FIFO में exactly 37 words हैं
यह "complete packet" है → PROCESS करो!
```

### Code में यही check — `Axi_IO.c` line 614:
```c
ReceiveLength = (XLlFifo_iRxGetLen(InstancePtr)) / WORD_SIZE;

if (ReceiveLength != RECEIVE_LENGTH) {
//  ↑ अगर 37 words नहीं आए → bad packet!
    // Buffer को empty करो, error count बढ़ाओ
    commGlitchCtr++;
} else {
    // 37 words आए → process करो!
    for (i = 0; i < RECEIVE_LENGTH; i++) {
        RxWord = XLlFifo_RxGetWord(InstancePtr);
        // ...store करो...
    }
}
```

---

# 🎬 पूरी Story एक बार फिर — अब ज्यादा Detail के साथ

```
STEP 1: RTDS simulation चल रही है
        हर simulation step पर (~125 µs assumed) एक नया step → 37 values calculate होती हैं
        ये सब pack होकर एक packet बनता है (148 bytes)

STEP 2: RTDS Aurora protocol में encode करता है
        Bits serial हो जाती हैं → fiber optic cable पर भेजता है
        010110001011110001...010010... (serial bit stream)

STEP 3: Kria के FPGA में Aurora IP receive करता है
        Serial bits → decode → 32-bit words बनाता है
        फिर AXI4-Stream format में convert करता है:
        
        Word 1: [DATA=0x00000001] [TLAST=0] → FIFO में
        Word 2: [DATA=0x3FA00000] [TLAST=0] → FIFO में
        Word 3: [DATA=0x41300000] [TLAST=0] → FIFO में
        ...
        Word 37:[DATA=0x00000064] [TLAST=1] → FIFO में ← LAST!

STEP 4: FIFO IP Core को TLAST=1 मिला
        "Packet complete!" → RC Interrupt flag set होता है
        A53 CPU को signal जाता है: "DATA READY!"

STEP 5: A53 CPU (जो idle था) INTERRUPT handle करता है
        FifoHandler() → FifoRecvHandler() → axiReceive()
        
        axiReceive() FIFO से एक-एक word पढ़ता है:
        XLlFifo_iRxOccupancy() → "हाँ data है"
        XLlFifo_iRxGetLen()    → "37 words हैं"
        
        Loop: i=0 to 36:
          RxWord = XLlFifo_RxGetWord()  ← FIFO से word निकालो
          rxData[i]->ptr पर store करो  ← सही variable में डालो
          (type conversion + scale factor apply)

STEP 6: Controller चलता है
        PEBB_Control_step() — calculations होती हैं
        Switch duty cycles, state commands calculate होते हैं

STEP 7: TX data FIFO में डालो → RTDS को वापस भेजो
        axiSend() → 23 words → Aurora TX → Fiber → RTDS
        
STEP 8: वापस IDLE — अगले RTDS packet का इंतज़ार
```

---

# 🧩 Bonus: तुम्हारे specific sub-questions के जवाब

### ❓ "RTDS 10 packets एक साथ भेज सकता है?"

**नहीं।** RTDS एक **real-time system** है।  
यह हर simulation step पर exactly 1 packet भेजता है (rate set by RTDS model). Buffer नहीं करता।  
RTDS का design ही ऐसा है कि — "every step, one packet."

### ❓ "Line by line भेजता है?"

**Technically हाँ** — Aurora over fiber optic पर bits serial आते हैं।  
लेकिन **logically नहीं** — Aurora IP उन्हें एक complete packet की तरह handle करता है।  
FIFO को TLAST तक का wait करना होता है।

### ❓ "FIFO में एक जगह = कितना data?"

```
एक "slot" = 1 word = 32 bits = 4 bytes

FIFO depth examples:
  - 512 word depth  = 2 KB total
  - 1024 word depth = 4 KB total
  - 4096 word depth = 16 KB total

हमारा 1 packet = 37 words = 148 bytes
हमारी FIFO depth = हम config से जानेंगे (xparameters.h में होगा)
लेकिन clearly 37 >> FIFO size है — overflow का कोई risk नहीं
```

### ❓ "Complete Packet = किसी length का हो सकता है?"

**RTDS के case में — नहीं।** हमेशा 37 words।  
लेकिन AXI4-Stream protocol में theoretically packet कोई भी length का हो सकता है।  
TLAST signal packet boundary बताता है — 1 word का packet भी हो सकता है, 10000 का भी।

---

# 📌 Summary Table — एक नज़र में सब कुछ

| Concept           | Detail                                                      |
| ----------------- | ----------------------------------------------------------- |
| RTDS packet size  | **37 words = 148 bytes** (fixed, हमेशा)                       |
| RTDS packet rate  | RTDS model step rate (assumed ~125 µs / 8 kHz; pending RTDS docs citation) |
| Aurora job        | Serial bits → 32-bit words → AXI4-Stream                    |
| AXI Stream        | FPGA के अंदर data transfer का highway (TDATA + TVALID + TLAST) |
| FIFO type         | AXI4-Stream FIFO (Xilinx IP, PG080)                         |
| FIFO slot size    | 1 word = 32 bits = 4 bytes                                  |
| FIFO depth        | 512+ words (config में set)                                   |
| Interrupt trigger | जब TLAST=1 आता है → RC (Receive Complete) interrupt           |
| Interrupt handler | `FifoHandler` → `FifoRecvHandler` → `axiReceive`            |
| Data reading      | `XLlFifo_RxGetWord()` को 37 बार call करो                       |
| Complete packet   | Exactly 37 words, TLAST=1 के साथ आया हो                         |

---

## अब तुम क्या कर सकते हो?

1. यह doc पढ़ लो।
2. `docs/milestone2/M2-A_auroralink_findings.md` लिखो।
3. ऊपर के answers directly use करो — Push/Pull explanation, packet structure table, interrupt flow।
4. Source cite करो: `Axi_IO.h` (constants), `Axi_IO.c` (axiReceive), `main.c` (FifoHandler/FifoRecvHandler).