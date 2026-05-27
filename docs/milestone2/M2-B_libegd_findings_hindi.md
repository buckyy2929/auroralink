# M2-B — libEGD Protocol & API: पूरी समझ (Hindi में)



> **यह document `M2-B_libegd_findings.md` का Hindi mirror है।**  
> English doc reviewer/customer के लिए है। यह Hindi doc तुम्हारे खुद के लिए है — हर concept simple language में, train metaphor के साथ।

---

## 🚉 पहले एक कहानी — "Railway Announcement System"

> कल्पना करो: एक बड़ा Railway Station है — जैसे Mumbai Central। यहाँ हर 2 minute में एक नई train आ रही है।
>
> Station के **Control Room** में एक announcer है। उसका काम है: हर train के बारे में जानकारी (train number, platform, time, status) PA system पर announce करना। उसकी आवाज़ पूरे station में फैलती है — Platform 1, Platform 2, Platform 3 — सब जगह।
>
> Platforms पर बहुत सारे लोग हैं — कोई train 12055 का wait कर रहा है, कोई 09011 का। **जो भी सुनना चाहे, सुन ले।** Control Room को परवाह नहीं कि कौन सुन रहा है — वो बस announce करता रहता है।
>
> यही **EGD (Ethernet Global Data)** है।
>
> - **Control Room (announcer)** = **EGD Producer** (data भेजने वाला)
> - **PA system** = **UDP broadcast** (network पर सब तक पहुँचता है)
> - **Platforms पर लोग** = **EGD Subscribers** (data सुनने वाले)
> - **एक announcement** = **एक EGD message** (header + data)
> - **Train ka schedule book** = **EGD Config XML** (किस announcement में कौन सी field कहाँ है)

**Important सोच:** EGD में **Producer एक होता है, Subscribers कई हो सकते हैं।** Producer को feedback नहीं चाहिए — वो बस periodically (हर X milliseconds में) announcement करता रहता है।

---

## 📦 Part 1: EGD Protocol क्या है — पूरा Picture

EGD = **Ethernet Global Data** — GE Industrial का एक protocol है, जो UDP पर **cyclic data exchange** के लिए use होता है।

### Key Facts (एक नज़र में):

| Concept | Value | Simple Explanation |
|---|---|---|
| Transport | **UDP** | TCP नहीं — fire-and-forget, no acknowledgement |
| Data Port | **18246** | इस port पर actual data जाता है |
| Config Port | **7937** | XML schema इस port से HTTP GET से लेते हैं |
| Max Payload | **1400 bytes** | इससे बड़ा भेजना है तो multiple "exchanges" में तोड़ो |
| Header Size | **28 bytes** | हर message के शुरू में fixed header |
| Endianness | **Little-Endian** | Payload के numbers little-endian में encode होते हैं |
| Model | **Producer / Consumer** | एक भेजने वाला, कई सुनने वाले |

**मतलब Train की language में:**

```
Control Room  ── PA system (UDP broadcast) ──►  Platform 1 (Subscriber 1)
   (1 producer)                              ──►  Platform 2 (Subscriber 2)
                                             ──►  Platform 3 (Subscriber 3)
```

Producer एक `sendto()` call करता है UDP socket पर। Network वो packet सब subscribers तक पहुँचा देता है। कोई reply नहीं। कोई retransmit नहीं।

---

## 📦 Part 2: Message Format — एक Announcement कैसा दिखता है?

हर EGD message दो हिस्सों में बँटा होता है:

```
┌─────────────────────────┬───────────────────────────────────┐
│  DataProductionHdr      │       Data Payload                │
│      (28 bytes)         │       (up to 1400 bytes)          │
└─────────────────────────┴───────────────────────────────────┘
   "किसने कब क्या भेजा"        "actual data — voltages, currents, …"
```

### Header के 9 fields (28 bytes):

| Field | Size | Train metaphor | Real meaning |
|---|---|---|---|
| `PDUType` | 1 byte | "Announcement type" | हमेशा **13** (Data Production) |
| `Version` | 1 byte | "Protocol version" | हमेशा **1** |
| `ReqID` | 2 bytes | "Announcement serial number" | हर send पर +1 बढ़ता है |
| `ProducerID` | 4 bytes | "Station ID" | Producer का IP address as integer |
| `ExchangeID` | 4 bytes | "Page number" | कौन सा data page भेजा जा रहा है |
| `Timestamp` | 8 bytes | "वक्त" | `tv_sec` + `tv_nsec` (POSIX timespec) |
| `Status` | 2 bytes | "Train OK / cancelled" | 2 = healthy, 1 = invalid |
| `Signature` | 2 bytes | "Schedule book version" | Config का checksum |
| `ConfigTimeSecs` | 4 bytes | "Schedule जारी होने का time" | Config generation time |

**Critical समझ:** `Signature` field बहुत important है। Producer और Subscriber दोनों को same config XML use करना है। अगर Signatures match नहीं हुए तो subscriber को पता चल जाएगा कि config mismatch है — और वो message को `EGD_MESSAGE_SIG_ERROR` mark करेगा।

> **Train analogy:** अगर Control Room कहता है "Platform 5 पर Rajdhani" — लेकिन तुम्हारी schedule book कहती है "Platform 5 का मतलब है Shatabdi" — तो mismatch हो गया। Subscriber को announcement मिल भी जाए तो वो उसका मतलब नहीं समझ पाएगा।

### Payload (actual data) के बारे में:

Payload **binary** होता है — कोई text नहीं, JSON नहीं, XML नहीं। बस raw bytes। Layout क्या है — कौन सी field किस byte/bit offset पर है — वो **Config XML** बताता है।

---

## 📦 Part 3: Config XML — "Schedule Book"

EGD में payload का layout dynamic है — मतलब हर deployment अलग fields रख सकता है। तो कैसे पता चले कि byte 4 पर क्या है, byte 8 पर क्या है?

**जवाब:** Producer एक XML config publish करता है HTTP पर — port 7937 पर।

### Config fetch कैसे होता है:

```
GET http://<producer_host>:7937/EGD?Action=GetDoc
                                     &Type=ProducedData
                                     &Active=True
                                     &ProducerID=<id>
```

### Response XML का sample:

```xml
<Exchange ExchangeId="1" DataLength="36" Page="DG_OUT" PeriodNSecs="10000000">
    <Var Name="VOLTAGE_PHASE_A" DType="REAL"  VOffs="0"   />
    <Var Name="VOLTAGE_PHASE_B" DType="REAL"  VOffs="32"  />
    <Var Name="VOLTAGE_PHASE_C" DType="REAL"  VOffs="64"  />
    <Var Name="STATUS_OK"       DType="BOOL"  VOffs="192" />
</Exchange>
```

### इसे train language में पढ़ो:

| XML Attribute | Train metaphor |
|---|---|
| `Exchange` | एक प्रकार की announcement (e.g., "Arrival announcements" vs "Departure announcements") |
| `ExchangeId="1"` | इस तरह की announcement का unique ID |
| `DataLength="36"` | Payload में total 36 bytes आएँगे |
| `Page="DG_OUT"` | इस announcement का नाम |
| `PeriodNSecs="10000000"` | हर 10 ms में broadcast होता है (= 100 Hz) |
| `<Var Name="..." DType="REAL" VOffs="0">` | Payload में byte 0 पर एक `REAL` (= float32) field है जिसका नाम VOLTAGE_PHASE_A है |

**Critical detail:** `VOffs` एक **bit offset** है, byte नहीं! तो `VOffs="32"` का मतलब = `byte_offset = 32 / 8 = 4` (पाँचवाँ byte से शुरू)। `VOffs="192"` = byte 24। `BOOL` के लिए तो bit-level packing भी हो सकता है।

### Supported data types (`DType`):

| DType | Size | C type |
|---|---|---|
| `BOOL` | 1 bit | bool (bit-packed) |
| `BYTE` / `SINT` / `USINT` | 1 byte | int8 / uint8 |
| `WORD` / `INT` / `UINT` | 2 bytes | int16 / uint16 |
| `DWORD` / `DINT` / `UDINT` | 4 bytes | int32 / uint32 |
| `LWORD` / `LINT` / `ULINT` | 8 bytes | int64 / uint64 |
| `REAL` | 4 bytes | float (IEEE 754) |
| `LREAL` | 8 bytes | double |

सब multi-byte types **little-endian** में store होते हैं (libEGD `htole32` / `htole64` use करता है)।

---

## 📦 Part 4: libEGD API — कौन सी function कब use करें

libEGD हमें दो "client" classes देती है:

```
┌──────────────────────────────────────┐
│    EgdClient (raw bytes)             │  ← Low-level: तुम खुद bytes pack करो
├──────────────────────────────────────┤
│    JsonClient (parsed by name)       │  ← High-level: JSON keys में काम करो
└──────────────────────────────────────┘
       दोनों ही same underlying UDP socket use करते हैं।
       JsonClient internally EgdClient को call करता है।
```

### API Cheat Sheet:

| Function | Direction | Sync/Async | जब use करो |
|---|---|---|---|
| `EgdClient::Publish()` | TX | **Sync** | अगर तुम manually bytes pack कर रहे हो |
| `EgdClient::Subscribe()` | RX | **Async** | Background thread पर callbacks चाहिए |
| `EgdClient::SubscribeBlocking()` | RX | **Sync** | अगर तुम्हारा main thread सिर्फ EGD पढ़ रहा है |
| `JsonClient::Publish()` | TX | **Sync** | JSON string से fields decode करो |
| `JsonClient::Subscribe()` | RX | **Async** | JSON में data चाहिए |
| `JsonClient::SubscribeBlocking()` | RX | **Sync** | Main thread + JSON |
| `egd_json_client_*()` | — | — | C language के लिए wrapper functions |

### Sync vs Async — Train analogy से समझो:

| Mode | Train analogy |
|---|---|
| **Publish (Sync)** | तुम platform पर खड़े होकर खुद announce करते हो ("Train 12055 platform 3"). जब तुम बोल चुके, function return हो जाता है। कोई wait नहीं। |
| **Subscribe (Async)** | तुम एक listener hire करते हो जो हर announcement सुनता है। जब भी कोई announcement आए, वो तुम्हें message पर ping करता है। तुम कुछ और काम कर सकते हो। |
| **SubscribeBlocking** | तुम खुद platform पर खड़े हो जाते हो और हर announcement का wait करते हो। कुछ और नहीं कर सकते — बस सुनते रहो। |

**बहुत important detail:** `Subscribe()` के callbacks libEGD के **internal thread** से fire होते हैं। तो तुम्हारे callback में:
- **NO heavy work** — बस data को shared buffer में लिखो और return करो
- **NO blocking calls** — मतलब उस thread में file I/O, network call, mutex contention मत करो
- **NO long computations** — वो callback fast होना चाहिए, क्योंकि अगले message का wait उसी thread पर हो रहा है

> **Train analogy:** अगर listener हर announcement सुनकर 5 minutes तक तुम्हें phone पर समझाता रहे, तो अगली announcement miss हो जाएगी। उसे बस "नया announcement आया" बोलना है — detail तुम बाद में पढ़ोगे।

---

## 📦 Part 5: Publish Flow — एक Message कैसे भेजा जाता है (TX path)

जब application `JsonClient::Publish(host, port, page_info, json_string)` call करती है, तो internally यह 8 steps होते हैं:

```
Step 1: Application call —
        JsonClient::Publish("192.168.1.50", 18246, page_info, "{\"VOLTAGE_A\": 230.5}")
              │
Step 2: JSON parse —
        json::parse(json_string)  →  { "VOLTAGE_A": 230.5 }
              │
Step 3: हर JSON key के लिए —
        a. page_info.var_map में देखो: "VOLTAGE_A" कहाँ है?
        b. var.bit_offset / 8  =  byte_offset निकालो
        c. value को binary में convert करो:
              REAL → htole32(float_bits)
              LREAL → htole64(double_bits)
              BOOL → सही bit position में shift करो
        d. DataProductionMessage.Data[byte_offset] पर लिख दो
              │
Step 4: EgdClient::Publish() को call करो (अब binary data के साथ)
              │
Step 5: इस (host, port) pair के लिए पहली बार है क्या?
        अगर हाँ —
            a. socket(AF_INET, SOCK_DGRAM, 0)
            b. setsockopt(SO_BROADCAST)  ← broadcast enable करो
            c. EgdPubInfo को publisher_map_ में cache करो
              │
Step 6: Header build करो —
        PDUType = 13 (Data Production)
        Version = 1
        ProducerID = inet_addr(producer_ip)
        ExchangeID = page_info.exchange_id
        Timestamp = clock_gettime(CLOCK_REALTIME)
        Status, Signature = page_info से
        ReqID = इस call पर +1
              │
Step 7: sendto(socket, &message, sizeof(hdr)+data_len, ...)
              │
Step 8: Return EgdStatus::OK
```

**मतलब Train की language में:**

```
1. तुम Control Room को बोले: "230.5 voltage का announcement कर दो"
2. Control Room ने 230.5 को IEEE 754 float bits में convert किया
3. उसे payload के सही byte position पर लिखा (schedule book के मुताबिक)
4. PA system के announcement format में wrap किया (header)
5. PA system पर बोल दिया — खत्म!
```

---

## 📦 Part 6: Subscribe Flow — सुनने का काम (RX path)

जब application `JsonClient::Subscribe(host, port, page_info, callback, user_data)` call करती है:

### Setup phase (पहली बार):

```
Step 1: JsonClient internally EgdClient::Subscribe() को call करता है
              │
Step 2: इस (host, port) pair के लिए पहली बार है?
        अगर हाँ —
            a. socket(AF_INET, SOCK_DGRAM, 0)
            b. setsockopt(SO_REUSEADDR)  ← multiple subscribers एक ही port पर
            c. bind(socket, port=18246, host_ip)
            d. setsockopt(SO_RCVBUF) = 128 KB
            e. std::thread spawn करो → PollSocket() में जाए
            f. EgdSubInfo को subscriber_map_ में cache करो
              │
Step 3: तुम्हारे callback को sub_info.callbacks[] में add करो
              │
Step 4: Application को return — अब background thread काम कर रहा है
```

### Background thread का loop (हमेशा):

```
Loop forever:
   Step 5: poll(socket, timeout=1000ms)
                ↓ data आया तो ↓
   Step 6: recvfrom(socket, buffer, 1428)
                ↓
   Step 7: Validate message:
            - PDUType == 13 ? (अगर नहीं — drop)
            - Version == 1 ?
            - ProducerID matches ?
            - ExchangeID matches ?
            - Status field → VALID / UNHEALTHY
            - Signature matches → अगर नहीं → SIG_ERROR
                ↓
   Step 8: हर registered callback के लिए —
            JsonProxyCallback(page_info, header, status, data, len, user_data)
                ↓
   Step 9: हर var in page_info.var_map के लिए —
            byte_offset = var.bit_offset / 8
            data[byte_offset:byte_offset+N] को binary से typed value में convert
            Little-endian को host-endian में convert (le32toh/le64toh)
            json_message[var_name] = decoded_value
                ↓
   Step 10: user_callback(page_info, header, status, json_string, user_data)
                ↓
            (वापस loop के top पर)
```

**मतलब Train की language में:**

```
1. तुमने listener hire किया जो Platform 1 पर खड़ा है
2. हर announcement पर वो check करता है:
   - "क्या यह सही announcement type है?" (PDUType)
   - "क्या यह सही station से है?" (ProducerID)
   - "क्या यह जो schedule मुझे दी थी उसी की है?" (Signature)
3. अगर सब सही है — तो schedule book देखकर हर field को decode करता है
   ("byte 0 का REAL means VOLTAGE_PHASE_A")
4. Decoded JSON तुम्हें ping पर भेज देता है
```

---

## 📦 Part 7: हमारे Bridge के लिए Recommendation — JsonClient vs EgdClient

**हमारी choice: `JsonClient` use करो।** क्यों?

| Factor | JsonClient (हाँ) | EgdClient (नहीं) |
|---|---|---|
| Field encoding | **Automatic** — config XML से होता है | **Manual** — तुम खुद byte pack करो |
| Field naming | JSON keys = variable names | सिर्फ byte offsets — typo पकड़ना hard |
| Endianness | **Internally handle होता है** | तुम्हारी responsibility |
| Maintenance | High | Low (brittle) |
| Performance overhead | JSON parse/serialize per message | None |

### "Performance overhead" को seriously लेने की ज़रूरत है क्या?

Short answer: **नहीं, हमारे scenario में।** क्यों:

- Aurora-side rate (M2-A में "125 µs / 8 kHz" बताया गया है — लेकिन यह number **unsourced** है, M2-A Gap 10.2 देखो)। यह RTDS model से आता है, Aurora से नहीं।
- EGD-side rate config XML के `<Exchange PeriodNSecs="...">` से आता है। ControlST में typically 10 ms – 100 ms periods (10–100 Hz) होते हैं control data के लिए — **लेकिन authoritative rate तो customer का config XML बताएगा।**
- अगर Aurora 8 kHz पर है और EGD 100 Hz पर — तो EGD message भेजने में जो microseconds लगते हैं वो latency budget में negligible हैं।

> **Train analogy:** Train Driver (RTDS) हर 125 µs में एक train (packet) भेजता है। Announcement (EGD) हर 10 ms में एक होती है। तो announcement में jo time लगता है (JSON parse, sendto) वो उस 10 ms के budget में आराम से fit हो जाता है।

**Bottom line:** `JsonClient` use करो। अगर कभी profiling में दिखे कि JSON cost problem है, तब `EgdClient` raw bytes पर migrate कर सकते हैं — लेकिन यह करने की ज़रूरत नहीं पड़ेगी (मेरा estimate)।

---

## 📦 Part 8: Threading Model Summary

| Scenario | Mechanism | Thread |
|---|---|---|
| **Publish (TX)** | Direct `sendto()` | Caller's thread (कोई internal thread नहीं) |
| **Subscribe (RX) — async** | `poll()` loop background में | 1 thread per `(host, port)` pair |
| **Subscribe (RX) — blocking** | `poll()` loop caller's thread पर | Caller block रहता है forever |
| **Destructor cleanup** | `run_flag_ = false` → `thread.join()` | Subscriber threads next poll timeout पर exit (≤ 1 sec) |

**Bridge के लिए key implication:** Subscriber callback libEGD के internal thread पर fire होता है। तुम्हें callback के अंदर सिर्फ shared buffer में data लिखना है और तुरंत return करना है। कोई भी bridge processing main thread पर होगी।

---

## 📦 Part 9: M2-D के लिए Open Item — EGD Config XML Fetch

> यह section **M2-B का deliverable नहीं है।** यह M2-D (bridge architecture) के लिए handoff है। यहाँ इसलिए लिखा है ताकि bridge implementer के सामने यह सवाल खुले रूप में आए।

### Problem:

libEGD का `JsonClient` बिना populated `EgdPageInfo` (with `var_map`) के काम नहीं करेगा। और `var_map` तो config XML से बनता है — जो HTTP से fetch करना पड़ता है।

```
GET http://<host>:7937/EGD?Action=GetDoc&Type=ProducedData&Active=True&ProducerID=<id>
```

**Bridge को startup पर यह fetch करना MUST है** — कोई भी `Publish()` या `Subscribe()` call से पहले। Field map hard-code नहीं हो सकता।

### Open Decisions (M2-D को answer करना है):

| # | Question | क्यों M2-D का काम |
|---|---|---|
| 9.1 | Config server का address — hard-coded? CLI arg? env var? service discovery? | Deployment config — bridge architecture decide करे |
| 9.2 | Retry policy — fail-closed at startup? Stale cached config serve करो? Forever retry? | Bridge की operational behaviour |
| 9.3 | Startup ordering — Aurora init पहले, EGD config fetch बाद, या साथ-साथ? | Thread graph design |
| 9.4 | `ProducerID` कैसे derive करें — `inet_addr(producer_host)`; multi-NIC में कौन सा IP? | Bridge deployment |
| 9.5 | क्या दोनों directions fetch करने हैं — `Type=ProducedData` (bridge consume करेगा) AND `Type=ConsumedData` (bridge produce करेगा)? | Customer's EMS server config पर depend |
| 9.6 | Config XML को disk पर cache करें? Bridge boot हो जाए अगर EMS server briefly down हो? | Bridge persistence policy |

### M2-B का contribution यहाँ:

Protocol-level facts — URL, port 7937, XML क्या होना चाहिए — सब M2-B में answered हैं (ऊपर Part 3)। बाकी सब M2-D का काम है।

---

## 🧠 Summary — 6 Lines में याद करो

1. **EGD = Producer / Consumer** model on UDP — एक भेजने वाला, कई सुनने वाले। Train analogy: PA system पर announcement।
2. **हर message = 28-byte header + up to 1400-byte binary payload।** Payload का layout config XML बताता है।
3. **दो main ports:** 18246 (data), 7937 (config HTTP GET)।
4. **API choice:** `JsonClient` हमारे bridge के लिए सही है — automatic encoding, naming by JSON keys, endianness internally handle।
5. **Threading:** Publish sync (caller's thread), Subscribe async (per-`(host, port)` background thread)। Callback fast रखना ज़रूरी।
6. **Open M2-D item:** Config XML startup fetch — कैसे, कब, और failure पर क्या करो — bridge architecture decide करेगी।

---

## 🎯 अगला कदम — M2-D में क्या होगा

M2-D bridge architecture में यह सब use होगा:

- Bridge startup पर config XML fetch (Part 9 की 6 decisions)
- Aurora-side se data आये → canonical buffer में लिखो → EGD-side thread उसे `JsonClient::Publish()` से भेजे
- Reverse direction में `JsonClient::Subscribe()` से data ले → canonical buffer → Aurora TX

Detailed flow `docs/BRIDGE_SOFTWARE_DESIGN.md` और उसके Hindi mirror में है।

---

*Source files (libEGD-master):*
- `src/egd/client/egd_client.cpp` — `Publish()`, `Subscribe()`, `PollSocket()`
- `src/egd/client/json_client.cpp` — `JsonClient::Publish()`, `JsonProxyCallback()`
- `util/egd_spec.h` — `MAX_EGD_PAYLOAD = 1400`, `DATA_PORT = 18246`
- `EgdTest.h` — `EGD_MAX_DATA_SIZE = 1400`, `Data_Production_Hdr` struct
- `EgdSend.c` — standalone reference sender
- `README.md` — config fetch URL format

*English mirror: `M2-B_libegd_findings.md` (इसी folder में)*
