# PEBB CHIL Firmware — Complete Technical Explanation
## System: RTDS → Aurora → AXI FIFO → A53 CPU (Kria Board)
### `main.c` + `Axi_IO.c` — Full Step-by-Step Guide

> **Sourcing & scope notes — read first**
>
> 1. **Aurora variant.** The fiber link uses `aurora_8b10b` v11.1 (Xilinx PG046), **not** Aurora 64B/66B. Specifically: 1 serial lane, 4-byte (32-bit) lane width, 2 Gbps line rate, 125 MHz GT reference clock, 50 MHz `user_clk_out`, Framing mode, Duplex, Flow Mode = None. Source: hardware handoff `design_1.hwh` extracted from `top.xsa`; full parameter table in `docs/milestone2/M2-A_auroralink_findings.md` §2. Any older draft of this doc that called it "Aurora 64B/66B" is wrong — the clock math (2 Gbps × 8/10 ÷ 32 bits = 50 MHz user clock, confirmed by the hwh) only works for 8B/10B.
> 2. **Frame rate (125 µs / 8 kHz).** This rate is set by the RTDS power-system model, not by the Aurora link itself. The "125 µs / 8 kHz" figure that recurs throughout this document is the **assumed** rate used by the supplied CHIL reference; the authoritative RTDS docs section that confirms it has **not** yet been located in this repo (open item in `docs/milestone2/M2-A_auroralink_findings.md` Known Gaps). Treat every "125 µs" / "8 kHz" mention below as an order-of-magnitude expectation, not a measured fact, until either (a) the RTDS manual citation is added, or (b) the inter-RC-interrupt period is measured on a live board.
> 3. **PS environment.** The CHIL reference here is a Vitis bare-metal application (`xllfifo_interrupt_example_1`) that runs directly on A53 from RAM via `.elf`. The protocol-translator deliverable will instead run on **Ubuntu** on A53; the FIFO interrupt path will be re-implemented over UIO or a kernel module (see M2-A §9.6 and `bridge/include/aurora_adapter.h`). The C-level driver call shape (`XLlFifo_*` → `read()`/`mmap`) is the only thing that changes; the data path described in this document still applies.
> 4. **No RTDS hardware in the lab — verification strategy.** Throughout this document the data source is described as "RTDS sends 37 words to Kria". For the **demo / lab work this project is targeted at**, there is no RTDS hardware available. Verification is done via a **five-stage ladder** documented in `docs/milestone2/M2-A_auroralink_findings.md` §12 and `explaination_docs/KR260_auroralink.md` §8: (S1) Vivado RTL simulation; (S2) on-board pattern generator → AXI FIFO → IRQ → PS with Aurora bypassed; (S3) PMA Near-End Loopback through Aurora (requires the Aurora IP to be re-customised to expose `loopback[2:0]` — see §8.3 F1 in the KR260 doc, this is the gotcha that cost us time previously); (S4) optional external SFP+ fibre loopback; (S5) real RTDS (production, deferred). Stages S1+S2+S3 fully validate the design described in this document on real silicon WITHOUT an RTDS. Every mention of "RTDS sends …" below should be read as "the verified RX path (pattern gen in S2, Aurora-looped pattern gen in S3, real RTDS in S5) delivers …".
>
> **Reading Analogy (Keep This in Mind Throughout):**
> - **RTDS** = Train Driver sending a train at the simulation step rate (assumed ~125 µs)
> - **Aurora fiber link** = The railway track (8B/10B-coded 2 Gbps serial, see note 1)
> - **AXI FIFO** = The platform where the train waits
> - **Interrupt** = The platform bell ringing when train fully arrives
> - **A53 CPU** = The station master who acts only when the bell rings
> - **main.c** = Station master's instruction book
> - **Axi_IO.c** = The goods manifest (what goes in/out and where)

---

## Part 1: System Overview & Architecture

### 1.1 What Is This System?

There are two rooms communicating with each other:

| Room | Name | Role |
|------|------|------|
| Room 1 | RTDS Simulator (big PC) | Simulates a power grid — computes new voltages, currents, commands every ~125 µs and sends them out |
| Room 2 | Kria Board (our FPGA+Processor) | Real controller hardware that reads those values, runs the control algorithm, and sends switch commands back |

**The connection between them:** A fiber optic cable using the **Aurora 8B/10B** serial protocol — specifically `aurora_8b10b` v11.1 (Xilinx PG046), 1 lane × 4-byte lane width, 2 Gbps line rate, 125 MHz reference clock. Confirmed from the hardware handoff file `design_1.hwh` extracted from `top.xsa`; full parameter table in `docs/milestone2/M2-A_auroralink_findings.md` §2. **Not** Aurora 64B/66B — that is a separate Xilinx IP (PG074) used at 5–10 Gbps and is NOT what this design uses.

---

### 1.2 The Three Players

```
RTDS Simulator          Kria Board — FPGA (PL)              Kria Board — Processor (PS)
[Big Computer]  ──►   [Aurora IP + AXI FIFO]           ──►   [A53 CPU — our Software]
  "Send data"          "Receive, decode, buffer it"           "Read data, run controller"
```

| Player | What it is | Where it lives |
|--------|-----------|----------------|
| RTDS Simulator | Real-Time Digital Simulator — simulates power system | External PC |
| Aurora IP Core | FPGA IP block — receives serial fiber bits, converts to AXI Stream | PL (Programmable Logic / FPGA) |
| AXI-FIFO IP Core | Buffer/queue — stores words until CPU is ready | PL (Programmable Logic / FPGA) |
| A53 CPU | 64-bit ARM processor — runs our software | PS (Processing System) |
| main.c | Manager software — interrupt setup, flow control | PS (runs on A53) |
| Axi_IO.c | Data expert — reads/writes FIFO, type conversion | PS (runs on A53) |

---

### 1.3 Kria Board Layout — PL vs PS

```
┌─────────────────────────────────────────────┐
│              KRIA BOARD                     │
│                                             │
│  ┌──────────────────┐  ┌──────────────────┐ │
│  │   PL (FPGA)      │  │   PS (Processor) │ │
│  │                  │  │                  │ │
│  │  Aurora IP       │  │  main.c          │ │
│  │  AXI FIFO IP     │  │  Axi_IO.c        │ │
│  │                  │  │                  │ │
│  │  (Hardware —     │  │  (Software —     │ │
│  │   physically     │  │   A53 CPU        │ │
│  │   wired in       │  │   executes this) │ │
│  │   the chip)      │  │                  │ │
│  └──────────────────┘  └──────────────────┘ │
└─────────────────────────────────────────────┘
```

> **Key point:** PL = hardware (Aurora, FIFO) — these are not "code", they are physically configured logic gates.
> PS = software (main.c, Axi_IO.c) — these run as instructions on the A53 CPU.

---

### 1.4 How Code Gets to the Board — The `.elf` Process

```
Your PC (Windows):
├── src/main.c
├── src/Axi_IO.c
├── src/UART.c
└── src/codegen.h
         │
         │  Vitis IDE → Build Project
         │  (aarch64-none-elf-gcc cross-compiler)
         ▼
Debug/xllfifo_interrupt_example_1.elf   ← 666 KB binary
         │
         │  JTAG cable → Kria Board
         │  (or: jtag_boot.tcl script)
         ▼
Kria RAM ← .elf loaded here
         │
         ▼
A53 CPU → starts executing main() instruction by instruction
```

- `.elf` = **Executable and Linkable Format** — Linux/embedded equivalent of a Windows `.exe`
- Files do NOT run from hard disk — they are loaded into **RAM**, then CPU executes from there
- `XLlFifo_RxGetWord()` and similar functions come from Xilinx's BSP (Board Support Package) libraries — already compiled and linked into the `.elf`

---

### 1.5 File Roles at a Glance

| File | Runs Where | Role |
|------|-----------|------|
| `main.c` | A53 CPU (PS) | **Manager:** interrupt setup, FifoHandler, FifoRecvHandler, runControlCycle |
| `Axi_IO.c` | A53 CPU (PS) | **Data Expert:** axiReceive (37 words in), axiSend (23 words out), rxData[] mapping, packBits() |
| `Aurora IP` | FPGA (PL) | **Hardware:** receives fiber bits → converts to AXI Stream → feeds FIFO |
| `AXI FIFO IP` | FPGA (PL) | **Hardware:** stores words in queue, fires interrupt on packet complete, manages READ/WRITE pointers |

---

## Part 2: Data Transfer Mechanism — Push, Pull, or Interrupt?

This is the most important architectural question.

---

### 2.1 Option A: Polling (Pull) — Why We Don't Use It

```c
// POLLING example — our code does NOT do this
while(1) {
    if (data_available()) {
        read_data();
    }
    // CPU is stuck checking every loop iteration — wasteful!
}
```

**Problem:** CPU wastes 100% of its time checking "has data arrived yet?" — like going to your front door every 5 minutes to check if mail arrived. Not usable in a real-time 8 kHz system.

---

### 2.2 Option B: DMA/Hardware Push — Why We Don't Use It

DMA (Direct Memory Access) would have the hardware automatically write data directly into RAM without CPU involvement.

**Why not:** Our system needs the CPU to actively convert data types, apply scale factors, and validate the packet — tasks that require software logic, not just memory copying.

---

### 2.3 Option C: Interrupt-Driven (Our System) ✅

```
CPU is idle in while(1) doing background tasks
         │
         │  ← 125 µs later
         │
FIFO hardware detects: last word arrived (TLAST=1)
         │
         │  Electric signal → wire goes HIGH → GIC
         │
GIC (Interrupt Controller):
    "FIFO interrupt! Go run FifoHandler!"
         │
CPU saves its state, jumps to FifoHandler()
         │
FifoHandler → FifoRecvHandler → axiReceive → reads 37 words
         │
CPU returns to while(1) after handling interrupt
```

**Why this is best:** CPU is free for other tasks while waiting. The hardware tells the CPU **exactly when** to act — no wasted cycles, perfect for real-time 8 kHz operation.

---

### 2.4 The Complete One-Sentence Answer

> "The PL (Aurora IP core) streams data into an AXI-FIFO via hardware push; when a complete packet arrives (TLAST=1), the FIFO hardware asserts an RC interrupt to the A53 PS, which then executes `FifoRecvHandler` → `axiReceive` to pull each word from the FIFO using `XLlFifo_RxGetWord` — making this an **interrupt-driven, software-pull** model from the PS perspective."

---

## Part 3: Data Path Deep Dive — Aurora → AXI Stream → FIFO

### 3.1 What is AXI Stream?

**AXI = Advanced eXtensible Interface** — Xilinx/ARM's standard for chip-to-chip communication. Like USB for PCBs: any AXI device talks to any AXI bus.

**AXI-Stream** specifically: a one-directional, continuous data stream. No addresses — just sequential words flowing from Aurora IP → AXI FIFO.

```
                    Data flow (one direction only)
Aurora IP  ──────────────────────────────────────►  AXI FIFO

Every clock cycle one "beat" is transferred:

Beat 1:  [DATA=0x3F800000] [VALID=1] [TLAST=0]  ← middle of packet
Beat 2:  [DATA=0x40000000] [VALID=1] [TLAST=0]  ← middle of packet
Beat 3:  [DATA=0x3E000000] [VALID=1] [TLAST=0]  ← middle of packet
...
Beat 37: [DATA=0x00000064] [VALID=1] [TLAST=1]  ← LAST WORD! Packet complete!
```

---

### 3.2 AXI Stream Signals

| Signal | Meaning |
|--------|---------|
| `TDATA` | 32-bit actual data (1 word per beat) |
| `TVALID` | Sender says: "this data is valid right now" |
| `TLAST` | Sender says: "this is the final word of the packet" |

When `TLAST=1` is received by the FIFO IP core → **RC (Receive Complete) interrupt is generated** → A53 CPU wakes up.

---

### 3.3 FIFO Structure — Read and Write Pointers

```
FIFO Memory (like a water tank):

        ┌─────────────────────────┐
IN ──►  │  [slot 1]  = 0x3F8...  │  ← WRITE PTR starts here
        │  [slot 2]  = 0x400...  │
        │  [slot 3]  = 0x3E0...  │
        │  [slot 4]  = 0x410...  │
        │  [slot 5]  = 0x000...  │
        │  ...                    │
        │  [slot N]  = (empty)   │
        └──────────┬──────────────┘
                   │
               OUT ▼  ← READ PTR starts here
          (CPU reads from here)
```

- **WRITE PTR** advances when Aurora writes a word in
- **READ PTR** advances when CPU calls `XLlFifo_RxGetWord()`
- `WRITE PTR > READ PTR` → FIFO has data (occupancy > 0)
- `WRITE PTR == READ PTR` → FIFO is empty

After reading all 37 words: `READ PTR = WRITE PTR = 37` → FIFO empty.

---

### 3.4 FIFO Capacity

| Attribute | Value |
|-----------|-------|
| Slot size | 1 word = 32 bits = 4 bytes |
| Typical depth | 512–4096 words |
| Total capacity | 512 × 4 = 2 KB (minimum) |
| Our packet size | 37 words = 148 bytes |
| Packets that fit | ~13–14 packets (but we always process before next arrives) |

> In practice, the FIFO holds exactly **1 packet (37 words)** at a time — we process it before the next one arrives at 8 kHz.

---

### 3.5 What Is a "Complete Packet"?

RTDS always sends exactly **37 words = 148 bytes** per simulation step.

```
Complete packet (correct):
RTDS sends words 1–37 → all arrive → TLAST=1 received
FIFO has exactly 37 words → PROCESS it ✓

Incomplete packet (error — can happen if connection glitches):
RTDS sends words 1–20 → connection drops → words 21–37 never arrive
FIFO has only 20 words → ReceiveLength != 37 → DISCARD it, increment commGlitchCtr
```

```c
// Axi_IO.c line 614
ReceiveLength = (XLlFifo_iRxGetLen(InstancePtr)) / WORD_SIZE;

if (ReceiveLength != RECEIVE_LENGTH) {  // != 37
    // drain FIFO, log error, do not process
    commGlitchCtr++;
    if (commGlitchCtr > COMM_GLITCH_LIMIT) {
        CHIL_error |= 1;
    }
}
```

---

### 3.6 The 37-Word Packet — Full Structure

RTDS sends this packet every ~125 µs (8 kHz):

| Word | Signal | Type | Scale | Description |
|------|--------|------|-------|-------------|
| 1 | Key | — | — | Security/sync key (ignored) |
| 2 | i_high | float | ×1000 (kA→A) | Phase A high-side current |
| 3 | vhigh_B | float | ×1000 (kV→V) | Phase B AC voltage |
| 4 | vhigh_A | float | ×1000 (kV→V) | Phase A AC voltage |
| 5 | v_dcc | float | ×1000 | DC capacitor voltage |
| 6 | i_low | float | ×1000 | Low side current |
| 7 | (dummy) | — | — | Unused |
| 8 | PEBB_NEXT_STATE_CMD | float | — | Next state command |
| 9 | P_CMD | float | ×1e6 (MW→W) | Active power command |
| 10 | LOWSIDE_CONTACTOR_CMD | float | — | Contactor command |
| 11 | PEBB_POWER_LOWER_LIMIT | float | ×1e6 | Power lower limit |
| 12 | PEBB_POWER_UPPER_LIMIT | float | ×1e6 | Power upper limit |
| 13–14 | (dummy) | — | — | Unused |
| 15 | ctr | s32 | — | Sub-step counter |
| 16–19 | (dummy) | — | — | Unused |
| 20 | V_CMD | float | — | Voltage command |
| 21 | ForceFault | float | — | Force fault command |
| 22–25 | (dummy) | — | — | Unused |
| 26 | EN_GRID_SUPPORT | float | — | Grid support enable |
| 27 | Q_CMD | float | ×1e6 (MVA→VA) | Reactive power command |
| 28 | Q_LIMIT | float | ×1e6 | Reactive power limit |
| 29 | i_high_B | float | ×1000 | Phase B current |
| 30 | i_high_C | float | ×1000 | Phase C current |
| 31 | v_high_c | float | ×1000 | Phase C voltage |
| 32 | v_stator_A | float | ×1000 | Stator voltage A |
| 33 | v_stator_B | float | ×1000 | Stator voltage B |
| 34 | v_stator_C | float | ×1000 | Stator voltage C |
| 35 | run | s32 | — | 1=run controller, 0=stop |
| 36 | RXI_chil | s32 | — | Closed-loop flag |
| 37 | seq | s32 | — | Sequence number (1–1000) |

```c
// Axi_IO.h
#define RECEIVE_LENGTH  37   // RTDS → Kria (always fixed)
#define TRANSMIT_LENGTH 23   // Kria → RTDS (controller outputs)
#define WORD_SIZE        4   // bytes per 32-bit word
```

---

## Part 4: Scene-by-Scene Code Flow

---

### Scene 1 — SETUP (Runs Once at Boot)

**What happens:** `main()` is called when the `.elf` loads. Everything is initialized before any data arrives.

---

#### Step 1A — `main()` calls `setupUART()`

```c
// main.c  Line 177–184
int main()
{
    int Status;

    if (setupUART() == XST_FAILURE) {
        return XST_FAILURE;
    }

    xil_printf("--- Entering main() ---\n\r");
```

- UART = serial port = console terminal (keyboard/screen connection)
- Must be set up first so `xil_printf()` debug prints work
- Analogy: "Turn on the intercom before making any announcements"

---

#### Step 1B — `main()` calls `XLlFifoInterruptExample()`

```c
// main.c  Line 188–193
while (1) {
    Status = XLlFifoInterruptExample(&FifoInstance, FIFO_DEV_ID);
    if (Status != XST_SUCCESS) break;
}
```

- `&FifoInstance` = pointer to our FIFO hardware object (defined at Line 132)
- `FIFO_DEV_ID` = hardware ID — tells the driver which FIFO chip to use
- The `while(1)` wrapper allows retry if FIFO fails and returns (normally runs forever inside)

---

#### Step 1C — Inside `XLlFifoInterruptExample()` — Hardware Init

```c
// main.c  Line 244–258
Config = XLlFfio_LookupConfig(DeviceId);
if (!Config) {
    xil_printf("No config found for %d\r\n", DeviceId);
    return XST_FAILURE;
}

Status = XLlFifo_CfgInitialize(InstancePtr, Config, Config->BaseAddress);
if (Status != XST_SUCCESS) {
    xil_printf("Initialization failed\n\r");
    return Status;
}
```

- `XLlFfio_LookupConfig()` → looks up the hardware address table for this FIFO chip (returns base address, etc.)
- `XLlFifo_CfgInitialize()` → resets all FIFO registers to default, maps physical hardware address so software can access it
- The FIFO is in the **PL (FPGA)**. CPU accesses it via **memory-mapped registers** — reading/writing a specific memory address IS reading/writing to the FIFO hardware.

---

#### Step 1D — Clear Old Interrupts

```c
// main.c  Line 261–269
Status = XLlFifo_Status(InstancePtr);
XLlFifo_IntClear(InstancePtr, 0xffffffff);  // clear ALL interrupt flags
Status = XLlFifo_Status(InstancePtr);
if(Status != 0x0) {
    xil_printf("\n ERROR : Reset value of ISR0 : 0x%x\t Expected : 0x0\n\r",
               XLlFifo_Status(InstancePtr));
    return XST_FAILURE;
}
```

- Clears any stale interrupt flags left over from a previous run
- `0xffffffff` = all bits set = clear every flag at once
- Then verifies the status register is now zero (clean)
- Analogy: "Before opening the station, make sure no old bells are still ringing"

---

#### Step 1E — Setup Interrupt System ← THE MOST IMPORTANT STEP

```c
// main.c  Line 275–279
Status = SetupInterruptSystem(&Intc, InstancePtr, FIFO_INTR_ID);
if (Status != XST_SUCCESS) {
    xil_printf("Failed intr setup\r\n");
    return XST_FAILURE;
}
```

This calls `SetupInterruptSystem()` which performs 5 sub-steps:

**Sub-step i — Initialize the GIC (General Interrupt Controller)**
```c
// main.c  Line 601–610
IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
                IntcConfig->CpuBaseAddress);
```
- GIC = the hardware chip that manages all interrupts on the SoC
- Finds GIC's memory address, initializes the driver
- Analogy: "Wake up the bell manager and get him ready"

**Sub-step ii — Set Priority and Trigger Type**
```c
// main.c  Line 612
XScuGic_SetPriorityTriggerType(IntcInstancePtr, FifoIntrId, 0xA0, 0x3);
```
- `0xA0` = priority level (lower number = higher priority)
- `0x3` = rising-edge triggered (interrupt fires when signal goes LOW→HIGH)
- Analogy: "Tell the bell manager: FIFO bell is important, listen for it going HIGH"

**Sub-step iii — Register `FifoHandler` as the Callback ← KEY LINE**
```c
// main.c  Line 619–621
Status = XScuGic_Connect(IntcInstancePtr, FifoIntrId,
                (Xil_InterruptHandler)FifoHandler,
                InstancePtr);
```
- Tells GIC: "When FIFO interrupt fires → call `FifoHandler()` function"
- `FifoHandler` is a **function pointer** — the memory address of our handler
- `InstancePtr` = FIFO instance pointer, passed as argument when FifoHandler is called
- Analogy: "Tell the bell manager: when the FIFO bell rings, call THIS specific person"

**Sub-step iv — Enable the Interrupt in GIC**
```c
// main.c  Line 626
XScuGic_Enable(IntcInstancePtr, FifoIntrId);
```
- Without this, GIC knows about the interrupt but silently ignores it
- Analogy: "Turn on the FIFO bell switch in the bell panel"

**Sub-step v — Register with the CPU Exception Table**
```c
// main.c  Line 637–644
Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
    (Xil_ExceptionHandler)INTC_HANDLER,
    (void *)IntcInstancePtr);

Xil_ExceptionEnable();
```
- The A53 CPU has an **exception vector table** — "what to do when X happens"
- We tell CPU: "When ANY interrupt arrives, call `INTC_HANDLER` (the GIC dispatcher)"
- GIC dispatcher then looks up which specific interrupt fired → calls the registered callback
- Analogy: "Tell the Station Master: if ANY bell rings, check with the bell manager first"

---

#### Step 1F — Enable FIFO Interrupts

```c
// main.c  Line 281
XLlFifo_IntEnable(InstancePtr, XLLF_INT_ALL_MASK);
```

- Enables ALL types of FIFO interrupts: TX complete, RX complete, errors
- `XLLF_INT_ALL_MASK` = bitmask with all bits set
- Analogy: "Tell the FIFO hardware: you are now allowed to ring any bell"

---

#### Step 1G — Initialize Controller + Enter the Waiting Loop

```c
// main.c  Line 284–294
setPEBBLRU();                    // initialize LRU count
PEBB_Control_initialize(initTime); // initialize the controller model
initPEBB();                      // initialize PEBB hardware

xil_printf("Paused. Please Start RTDS model now. \n\r");

while(1) {          // ← CPU SITS HERE WAITING
    readUART();     // check for UART console commands
    // check errors, print diagnostics...
}
```

- `PEBB_Control_initialize()` = initialize the PLECS/Simulink auto-generated control model
- `initPEBB()` = initialize hardware (gate drivers, contactors, etc.)
- CPU then enters **infinite loop** doing only lightweight background tasks
- **All real work happens in interrupts** — CPU does nothing heavy in this loop

> **Scene 1 Summary:** Program starts → UART setup → FIFO hardware init → GIC registers `FifoHandler` → interrupts enabled → controller init → CPU enters infinite wait loop. Everything from here is interrupt-triggered.

---

### Scene 2 — RTDS DATA ARRIVES — INTERRUPT FIRES!

**What happens:** RTDS sends 37 words every 125 µs (8 kHz). The packet travels:
`RTDS → fiber → Aurora IP (FPGA) → AXI Stream → AXI FIFO (PL)`

When the **37th and final word** arrives (with `TLAST=1`), the FIFO hardware:
1. Sets the **RC (Receive Complete)** flag in its interrupt status register
2. Sends an **electric signal** (wire goes HIGH) to the GIC

```
┌─────────────────────────────────────────────────────────────────┐
│  HARDWARE SIDE (PL - FPGA)          SOFTWARE SIDE (PS - CPU)   │
│                                                                 │
│  RTDS sends 37 words                                            │
│       ↓                                                         │
│  Aurora IP receives via fiber                                   │
│  (serial bits → 32-bit words → AXI Stream)                     │
│       ↓                                                         │
│  AXI FIFO stores words [1..37]                                  │
│       ↓                                                         │
│  Word 37 arrives with TLAST=1                                   │
│       ↓                                                         │
│  FIFO sets RC flag in ISR register                              │
│       ↓                                                         │
│  Electric signal → wire goes HIGH ──────────────────► GIC      │
│                                                         ↓       │
│                                             GIC sees FIFO intr  │
│                                                         ↓       │
│                                              CPU interrupted!   │
│                                              (while loop paused)│
│                                                         ↓       │
│                                              FifoHandler()      │
│                                              is called!         │
└─────────────────────────────────────────────────────────────────┘
```

**How Aurora processes the packet:**

```
Fiber Optic Cable
     │  (serial bits: 010110001011100010...)
     ▼
Aurora 8B/10B IP Core (PL) — `aurora_8b10b` v11.1 (PG046):
  1. Decode serial bits at 2 Gbps line rate
  2. Reverse the 8b/10b symbol mapping (10-bit symbols → 8-bit bytes), de-scramble
  3. Reconstruct 32-bit data words (4 bytes per word per `C_LANE_WIDTH=4`)
  4. Output as AXI4-Stream beats on `user_clk_out` (50 MHz)
     │
     │  Beat 1:  [TDATA=word1]  [TLAST=0]
     │  Beat 2:  [TDATA=word2]  [TLAST=0]
     │  ...
     │  Beat 37: [TDATA=word37] [TLAST=1]  ← PACKET COMPLETE!
     ▼
AXI FIFO
```

> **Key point:** Aurora processes one word per clock cycle (32 or 64 bits wide). But from software's perspective, we only care about the full packet arriving — Aurora handles the serial-to-parallel conversion transparently.

The CPU was in `while(1)` doing `readUART()`. The GIC taps the CPU: **"FIFO interrupt! Run FifoHandler!"** The CPU saves its state (registers, program counter) and jumps to `FifoHandler()`. This happens **entirely in hardware** — no polling.

---

### Scene 3 — `FifoHandler()` — Which Interrupt Type?

**What happens:** `FifoHandler()` is called by the GIC. It must determine **what type** of interrupt fired. FIFO can generate multiple interrupt types:

| Flag | Meaning |
|------|---------|
| RC | Receive Complete — data arrived (what we care about) |
| TC | Transmit Complete — our outgoing data was sent |
| Error | Something went wrong (overflow, underflow, etc.) |

```c
// main.c  Line 428–456
static void FifoHandler(XLlFifo *InstancePtr)
{
    u32 Pending;

    // Step 1: Ask the FIFO: "Which interrupt flag is set?"
    Pending = XLlFifo_IntPending(InstancePtr);

    while (Pending) {   // loop in case multiple flags are set simultaneously

        if (Pending & XLLF_INT_RC_MASK) {
            // RC flag is set → data received!
            XLlFifo_IntClear(InstancePtr, XLLF_INT_RC_MASK); // acknowledge/clear

            if (Rx_Done) {
                intOverrunFlag = 1; // WARNING: previous cycle not finished yet!
            }
            FifoRecvHandler(InstancePtr); // → go handle received data

        } else if (Pending & XLLF_INT_TC_MASK) {
            // TC flag → transmit complete
            XLlFifo_IntClear(InstancePtr, XLLF_INT_TC_MASK);
            if (Tx_Done) {
                intOverrunFlag = 1;
            }
            FifoSendHandler(InstancePtr);

        } else if (Pending & XLLF_INT_ERROR_MASK) {
            // Error happened
            FifoErrorHandler(InstancePtr, Pending);
            XLlFifo_IntClear(InstancePtr, XLLF_INT_ERROR_MASK);

        } else {
            XLlFifo_IntClear(InstancePtr, Pending); // clear unknown flags
        }

        // Check again — more flags pending?
        Pending = XLlFifo_IntPending(InstancePtr);
    }
}
```

**Line-by-line breakdown:**

| Code | What it does |
|------|-------------|
| `XLlFifo_IntPending()` | Reads ISR (Interrupt Status Register) — returns which flags are set |
| `Pending & XLLF_INT_RC_MASK` | Bitwise AND — checks if the RC bit is specifically set |
| `XLlFifo_IntClear(...RC_MASK)` | Writes 1 to RC bit in ISR to clear/acknowledge it |
| `if (Rx_Done) intOverrunFlag=1` | Previous data wasn't processed — **timing overrun!** |
| `FifoRecvHandler(InstancePtr)` | Calls the receive handler to process the data |

> **Why clear the interrupt flag BEFORE processing?** If we don't clear it, the FIFO keeps re-triggering the interrupt forever. The hardware keeps the flag set until software explicitly acknowledges it by writing 1 to it.

> **`intOverrunFlag`** is set when `Rx_Done=1` (previous cycle not done) but a new interrupt fires. This means the system is running slower than 8 kHz — a **timing overrun** problem requiring investigation.

---

### Scene 4 — `FifoRecvHandler()` — Orchestrate Processing

**What happens:** Short function that sequences three operations.

```c
// main.c  Line 470–480
static void FifoRecvHandler(XLlFifo *InstancePtr)
{
    // 1. Read all 37 words from FIFO into variables
    axiReceive(InstancePtr);   // → Axi_IO.c

    // 2. Mark that receive is done
    Rx_Done = 1;

    // 3. Update how many LRUs are connected
    setPEBBLRU();

    // 4. Run the control algorithm + transmit response
    if (runControlCycle(InstancePtr) == XST_FAILURE) {
        CHIL_error |= 1;
    }
}
```

| Call | What it does |
|------|-------------|
| `axiReceive()` | Reads 37 words from FIFO, converts types, stores in controller variables (in Axi_IO.c) |
| `Rx_Done = 1` | `volatile` flag — signals "this cycle's data is now in RAM" |
| `setPEBBLRU()` | Updates count of active LRUs (Line Replaceable Units) based on received data |
| `runControlCycle()` | Runs the PEBB controller + transmits 23 words back to RTDS |

> `volatile int Rx_Done` — the `volatile` keyword tells the compiler: "this variable can change at any moment from an interrupt, do NOT cache it in a CPU register."

---

### Scene 5 — `axiReceive()` — Reading All 37 Words (READ PTR Moves Here!)

This is the **heart of Axi_IO.c**. This is where the READ pointer advances.

```c
// Axi_IO.c  Line 604–684
void axiReceive(XLlFifo *InstancePtr)
{
    int i;
    u32 RxWord;
    seq_prev = seq;  // save previous sequence number for validation

    // ─── STEP 1: Does FIFO have any data? ────────────────────────
    if (XLlFifo_iRxOccupancy(InstancePtr)) {

        // ─── STEP 2: Read packet length ──────────────────────────
        ReceiveLength = (XLlFifo_iRxGetLen(InstancePtr)) / WORD_SIZE;
        // XLlFifo_iRxGetLen() returns bytes → divide by 4 → word count
        // Expected: 37 words

        // ─── STEP 3: Wrong size packet — drain and discard ───────
        if (ReceiveLength != RECEIVE_LENGTH) {
            for (i = 0; i < ReceiveLength; i++) {
                RxWord = XLlFifo_RxGetWord(InstancePtr); // drain (READ PTR moves!)
            }
            seq = 0;
            xil_printf("Warning: receive Length mismatch...");
            commGlitchCtr++;
            if (commGlitchCtr > COMM_GLITCH_LIMIT) {
                CHIL_error |= 1;
            }

        // ─── STEP 4: Correct size — process all 37 words ─────────
        } else {
            type_ thisAxiType;
            type_ thisLocalType;
            float unitScale;
            float RxValue;
            int isNan;

            for (i = 0; i < RECEIVE_LENGTH; i++) {  // i = 0 to 36

                // ← READ WORD i FROM FIFO (READ PTR ADVANCES BY 1 HERE!)
                RxWord = XLlFifo_RxGetWord(InstancePtr);

                // Look up this word's destination and type
                thisAxiType   = rxData[i]->axiType;    // float_? s32_? u32_?
                thisLocalType = rxData[i]->localType;  // double_? s32_?
                unitScale     = rxData[i]->scaleFactor; // 1? 1000?

                isNan = 0;

                // Check for NaN (corrupted float data)
                if (thisAxiType == float_) {
                    RxValue = (*(float *) &RxWord);
                    if (RxValue != RxValue) { // NaN: NaN != NaN is always true
                        isNan = 1;
                    }
                }

                if (isNan) {
                    CHIL_error |= 1;
                    NanRxCtr++;

                // Case 1: Same type both sides, no scaling needed → direct copy
                } else if ((thisAxiType == u32_ || thisAxiType == s32_ || thisAxiType == float_)
                        && thisLocalType == thisAxiType && unitScale == 1) {
                    u32* rxPtr = (u32 *) rxData[i]->ptr;
                    *rxPtr = RxWord;  // copy raw 32-bit word directly

                // Case 2: float on wire → float in RAM, with scaling
                } else if (thisAxiType == float_ && thisLocalType == float_) {
                    float* rxPtr = (float*) rxData[i]->ptr;
                    RxValue = (*(float *) &RxWord);
                    *rxPtr = RxValue * unitScale;

                // Case 3: float on wire → double in RAM (most common!)
                } else if (thisAxiType == float_ && thisLocalType == double_) {
                    double* rxPtr = (double*) rxData[i]->ptr;
                    RxValue = (*(float *) &RxWord); // reinterpret bytes as float
                    *rxPtr = (double) RxValue * unitScale; // cast to double, scale
                }
            }
        }

    } else {
        xil_printf("Error: RX interrupt with no RX FIFO Occupancy");
        CHIL_error |= 1;
    }
}
```

---

#### How the READ PTR Moves

```
FIFO Memory before axiReceive():
┌──────┬──────┬──────┬──────┬──────┬──────┐
│Word 1│Word 2│Word 3│ ...  │Word36│Word37│
└──────┴──────┴──────┴──────┴──────┴──────┘
↑                                          ↑
READ PTR (=0)                         WRITE PTR (=37)

After XLlFifo_RxGetWord() call #1:
  READ PTR = 1 → Word 1 consumed

After XLlFifo_RxGetWord() call #2:
  READ PTR = 2 → Word 2 consumed

... 37 calls later ...

READ PTR = 37 = WRITE PTR → FIFO is EMPTY ✓
```

`XLlFifo_RxGetWord()` is a Xilinx library function that:
1. Reads the data value from the FIFO's RX data register
2. The FIFO hardware **automatically advances the read pointer**
3. Software never touches the pointer manually — just call the function and the pointer advances

---

#### The `rxData[]` Array — The Address Book

Each of the 37 words has a descriptor struct telling us where to store it:

```c
// Axi_IO.h  Line 40–45
typedef struct {
    void*  ptr;          // WHERE to store this data (memory address)
    type_  localType;    // HOW it's stored locally (double, s32, float, etc.)
    type_  axiType;      // HOW it arrives on the AXI bus (float, s32, u32)
    float  scaleFactor;  // MULTIPLY by this after receiving
} axiData;
```

**Example — Word 2 (i_high, Phase A current):**
```c
// Axi_IO.c  Line 87–92
axiData rxData2 = {
    .ptr         = (void*) &PEBB_Control_U.feedback[0],  // store here
    .axiType     = float_,    // arrives as 32-bit IEEE 754 float
    .localType   = double_,   // store as 64-bit double in RAM
    .scaleFactor = UNIT_SCALE // multiply by 1000 (RTDS sends kA, we need A)
};
```

**Full rxData[] array — all 37 word destinations:**

```c
// Axi_IO.c  Line 573–578
volatile axiData *rxData[RECEIVE_LENGTH] = {
    &rxData1,  // Word 1:  key (ignored)
    &rxData2,  // Word 2:  feedback[0]  = i_high phase A current
    &rxData3,  // Word 3:  feedback[4]  = vhigh B phase AC voltage
    &rxData4,  // Word 4:  feedback[3]  = vhigh A phase AC voltage
    &rxData5,  // Word 5:  feedback[7]  = v_dcc DC capacitor voltage
    &rxData6,  // Word 6:  feedback[6]  = i_low current
    &rxData7,  // Word 7:  dummyDoubleRx (unused)
    &rxData8,  // Word 8:  PEBB_NEXT_STATE_CMD
    &rxData9,  // Word 9:  P_CMD (power command MW→kW)
    &rxData10, // Word 10: LOWSIDE_CONTACTOR_CMD
    &rxData11, // Word 11: PEBB_POWER_LOWER_LIMIT
    &rxData12, // Word 12: PEBB_POWER_UPPER_LIMIT
    &rxData13, // Word 13: dummy
    &rxData14, // Word 14: dummy
    &rxData15, // Word 15: ctr (substep counter)
    &rxData16, // Word 16: dummy
    &rxData17, // Word 17: dummy
    &rxData18, // Word 18: dummy
    &rxData19, // Word 19: dummy
    &rxData20, // Word 20: V_CMD (voltage command)
    &rxData21, // Word 21: ForceFault
    &rxData22, // Word 22: dummy
    &rxData23, // Word 23: dummy
    &rxData24, // Word 24: dummy
    &rxData25, // Word 25: dummy
    &rxData26, // Word 26: EN_GRID_SUPPORT
    &rxData27, // Word 27: Q_CMD (reactive power MVA→kVA)
    &rxData28, // Word 28: Q_LIMIT
    &rxData29, // Word 29: feedback[1]  = i_high_B
    &rxData30, // Word 30: feedback[2]  = i_high_C
    &rxData31, // Word 31: feedback[5]  = v_high_c
    &rxData32, // Word 32: feedback[8]  = v_stator_A
    &rxData33, // Word 33: feedback[9]  = v_stator_B
    &rxData34, // Word 34: feedback[10] = v_stator_C
    &rxData35, // Word 35: run (1=run controller, 0=stop)
    &rxData36, // Word 36: RXI_chil (close loop flag)
    &rxDataSeq // Word 37: seq (sequence number for packet verification)
};
```

---

#### Type Conversion — Type Punning Explained

The most confusing line in `axiReceive`:

```c
RxValue = (*(float *) &RxWord);
```

Step by step:
```
RxWord is u32 (unsigned 32-bit integer)
e.g. RxWord = 0x3F800000

&RxWord           → memory address of RxWord (a u32*)
(float *)&RxWord  → treat that same address as if it points to a float
*(float *)&RxWord → read the bytes at that address AS a float
                  = 1.0f   (0x3F800000 is IEEE 754 for 1.0)
```

This is **type punning** — we don't convert the value, we reinterpret the same raw bytes as a different type. RTDS sends floats as their raw IEEE 754 32-bit bit pattern, so we read them back the same way.

---

### Scene 6 — `runControlCycle()` — Controller + TX

```c
// main.c  Line 354–387
int runControlCycle(XLlFifo *InstancePtr)
{
    // ── STEP 1: STOP CHECK ────────────────────────────────────────
    if (!run && controllerStarted) {
        // RTDS sent run=0 after we were already running → STOP
        controllerStopped = 1;
        PEBB_Control_U.ForceFault = 1; // enter safe freewheeling mode
    }

    // ── STEP 2: RUN CONTROLLER ────────────────────────────────────
    //    Three conditions required simultaneously:
    //    1. ctrlOn = 1          (manually enabled via UART console)
    //    2. run = 1             (RTDS says run — word 35)
    //    3. ReceiveLength == 37 (valid packet received)
    if ((ctrlOn && run && ReceiveLength == RECEIVE_LENGTH) || controllerStopped) {
        controllerStarted = 1;

        if (ctrlCtr % PEBB_RATE == 0) {  // PEBB_RATE=1 → runs every cycle
            PEBB_Control_step();
            // ↑ Auto-generated PLECS/Simulink controller — one timestep
            // Reads:  PEBB_Control_U.feedback[] (currents/voltages from word 2–34)
            // Reads:  P_CMD, Q_CMD, V_CMD (power commands from words 9, 27, 20)
            // Writes: PEBB_Control_Y.switch_1[] (gate duty cycles)
            // Writes: PEBB_Control_Y.enable, not_fault, precharge, etc.
        }

        ctrlCtr++;
        if (ctrlCtr == CTR_MAX) ctrlCtr = 0; // CTR_MAX=8 → cycle 0–7
    }

    // ── STEP 3: TRANSMIT RESPONSE BACK TO RTDS ───────────────────
    if (TxSend(InstancePtr) != XST_SUCCESS) {
        xil_printf("Transmission of Data failed\n\r");
        return XST_FAILURE;
    }

    // ── STEP 4: DUMP ON STOP ──────────────────────────────────────
    if (run == 0 && runPrev == 1 && dumpOnStop == 1) {
        dumpFlag = 1; // request a debug data dump
    }

    // ── STEP 5: RESET FLAGS ───────────────────────────────────────
    runPrev = run;
    Rx_Done = Tx_Done = 0; // ← reset for next cycle
    return XST_SUCCESS;
}
```

**Variable reference:**

| Variable | Meaning | Set by |
|----------|---------|--------|
| `run` | 1=run controller, 0=stop | rxData35 (Word 35 from RTDS) |
| `ctrlOn` | Manual enable/disable | UART console command |
| `ReceiveLength` | Count of words received | axiReceive() |
| `controllerStarted` | Controller has run at least once | runControlCycle() |
| `controllerStopped` | run went 1→0 after starting | runControlCycle() |
| `PEBB_Control_step()` | One step of auto-generated controller | PLECS/Simulink codegen |
| `ctrlCtr % PEBB_RATE` | Rate divider — run every N cycles | config |
| `Rx_Done = 0` | Reset flag for next interrupt cycle | runControlCycle() end |

`TxSend()` calls `axiSend()` in Axi_IO.c → puts 23 words in TX FIFO → sends back to RTDS.

---

## Part 5: TX Path — Sending Data Back to RTDS

### 5.1 `axiSend()` — Writing 23 Words to TX FIFO

```c
// Axi_IO.c  Line 702–754
void axiSend(XLlFifo *InstancePtr)
{
    packedInt = packBits(); // pack 8 binary flags into one u32 word

    for(i = 0; i < TRANSMIT_LENGTH; i++) {  // 23 words

        if (XLlFifo_iTxVacancy(InstancePtr)) { // space available in TX FIFO?

            // Most common case: double in RAM → float on wire
            if (thisAxiType == float_ && thisLocalType == double_) {
                float txWord = unitScale * (float)(*(double*)txData[i]->ptr);
                XLlFifo_TxPutWord(InstancePtr, *(u32*)&txWord);
                // convert double→float, scale, reinterpret as u32, put in TX FIFO
            }
        }
    }

    // Tell FIFO: "Send everything you have (23 words × 4 bytes)"
    XLlFifo_iTxSetLen(InstancePtr, (TRANSMIT_LENGTH * WORD_SIZE));
    // ↑ THIS is what actually triggers the send — not TxPutWord
}
```

The TX path is the **reverse** of RX:
- RAM has `double` values (controller outputs)
- Cast to `float`, apply scale factor (inverse of RX scale)
- Reinterpret float bytes as `u32`, put into TX FIFO
- After all 23 words: call `XLlFifo_iTxSetLen()` to trigger transmission

---

### 5.2 `packBits()` — 8 Binary Signals in 1 Word

```c
// Axi_IO.c  Line 770–782
u32 packBits()
{
    u32 pack = 0;
    for (i = 0; i < NUM_PACKED_BITS; i++) {   // 8 bits
        if ((bits[i]->boolPtr != NULL && *(bits[i]->boolPtr) != 0) ||
                (bits[i]->doublePtr != NULL && *(bits[i]->doublePtr) != 0)) {
            pack = ((1 << bits[i]->bitPos) | pack); // set bit at bitPos
        }
    }
    return pack;
}
```

The 8 packed bits in `txData3`:

| Bit Position | Signal | Type |
|---|---|---|
| Bit 0 | `lowside_contact` | double |
| Bit 1 | `enable` | double |
| Bit 2 | `highside_contact` | double |
| Bit 3 | `not_fault` | bool |
| Bit 4 | `dummyBoolTx` | bool |
| Bit 5 | `dummyBoolTx` | bool |
| Bit 6 | `dummyBoolTx` | bool |
| Bit 7 | `precharge_contact` | double |

All 8 on/off signals packed into ONE 32-bit word → saves Aurora bandwidth.

---

### 5.3 CHIL Concept — Why We Must Send Data Back

**CHIL = Controller Hardware-In-the-Loop**

```
Real World Operation:
  Real Power Grid                 Controller (PEBB)
  (Actual hardware)               (Real hardware)
       │                                │
       │◄──── switch commands ──────────│
       │────── sensor values ──────────►│
  Grid reacts → new sensor values follow naturally

CHIL Testing:
  RTDS (FAKE simulated grid)      Kria Board (REAL controller code)
       │                                │
       │◄──── 23 words TX ─────────────│  switch commands (duty cycles)
       │────── 37 words RX ───────────►│  sensor values
  RTDS simulates: "given these switch states, what are the next voltages/currents?"
```

**Why TX is required — the closed loop:**
```
Every 125 µs:
RTDS → "i_high=1.5kA, v_dc=11kV, run=1" → Kria
                                              ↓
                                    PEBB_Control_step()
                                    "duty_cycle = 0.75"
                                              ↓
Kria → "switch_1[0]=0.75, enable=1, not_fault=1" → RTDS
                                              ↓
RTDS simulates: "If phase A switch is 75% ON..."
"→ next i_high = 1.523 kA, v_dc = 10.98 kV"
                                              ↓
RTDS → "i_high=1.523kA, v_dc=10.98kV, ..." → Kria (next cycle)
```

**If Kria does not send TX:** RTDS has no switch states to simulate. It cannot compute next sensor values. The simulation loop freezes.

> **CHIL advantage:** Testing a real controller against a simulated power grid is safe and cheap. Mistakes crash only software — no real hardware is damaged. This is done before real deployment.

---

## Part 6: Configuration Parameters

| Parameter | Value | Defined In | Purpose |
|-----------|-------|-----------|---------|
| `RECEIVE_LENGTH` | 37 | `Axi_IO.h` line 23 | Words per RX packet from RTDS |
| `TRANSMIT_LENGTH` | 23 | `Axi_IO.h` line 22 | Words per TX packet to RTDS |
| `WORD_SIZE` | 4 bytes | `Axi_IO.h` line 21 | 1 word = 32 bits = 4 bytes |
| `UNIT_SCALE` | 1000 | `Axi_IO.h` line 26 | kA/kV → A/V conversion factor |
| `SEQ_MAX` | 1000 | `Axi_IO.h` line 27 | Max RTDS sequence number (wraps) |
| `NUM_PACKED_BITS` | 8 | `Axi_IO.h` line 24 | Number of binary flags in packed word |
| `CTR_MAX` | 8 | `main.c` line 96 | Controller step counter cycles 0–7 |
| `PEBB_RATE` | 1 | `main.c` line 97 | Run controller every N interrupts |
| `COMM_GLITCH_LIMIT` | 10 | `Axi_IO.c` line 59 | Max allowed bad packets before error |
| `FIFO_DEV_ID` | HW param | `main.c` line 75 | FIFO hardware base address identifier |

> **RTDS side must be configured with the same values:** exactly 37 RX words and 23 TX words in its Aurora I/O list. Mismatch → "Warning: receive Length mismatch" in `axiReceive()`.

---

## Part 7: Complete Flow Diagram

```
PROGRAM START
     │
     ▼
[SCENE 1] main() → XLlFifoInterruptExample()
     │  setupUART()
     │  XLlFfio_LookupConfig() + XLlFifo_CfgInitialize()
     │  XLlFifo_IntClear(0xffffffff)
     │  SetupInterruptSystem():
     │     → XScuGic_CfgInitialize()
     │     → XScuGic_SetPriorityTriggerType()
     │     → XScuGic_Connect(FifoHandler)   ← REGISTER CALLBACK
     │     → XScuGic_Enable()
     │     → Xil_ExceptionRegisterHandler() + Xil_ExceptionEnable()
     │  XLlFifo_IntEnable(XLLF_INT_ALL_MASK)
     │  setPEBBLRU() + PEBB_Control_initialize() + initPEBB()
     │
     ▼
     while(1) {          ← CPU WAITS HERE
         readUART();
         check errors;
     }
     │
     │  ← 125 µs later, RTDS sends 37 words via fiber
     │
[SCENE 2] HARDWARE:
     │  Aurora receives 37 beats (TDATA + TLAST on beat 37)
     │  AXI FIFO stores all 37 words
     │  TLAST → RC flag → wire HIGH → GIC → CPU interrupted
     │
     ▼
[SCENE 3] FifoHandler() — called by GIC
     │  XLlFifo_IntPending() → read ISR → RC flag set!
     │  XLlFifo_IntClear(RC_MASK) → acknowledge interrupt
     │  (check intOverrunFlag if Rx_Done still 1)
     │  call FifoRecvHandler()
     │
     ▼
[SCENE 4] FifoRecvHandler()
     │  axiReceive()          ← reads 37 words from FIFO
     │  Rx_Done = 1
     │  setPEBBLRU()
     │  runControlCycle()
     │      ├─ PEBB_Control_step()    ← controller runs (if run=1, ctrlOn=1)
     │      ├─ TxSend() → axiSend()   ← 23 words back to RTDS
     │      └─ Rx_Done = Tx_Done = 0  ← reset for next cycle
     │
     ▼
[SCENE 5] axiReceive() — in Axi_IO.c
     │  XLlFifo_iRxOccupancy() → check FIFO not empty
     │  XLlFifo_iRxGetLen() / WORD_SIZE → ReceiveLength
     │  if ReceiveLength != 37 → drain + error
     │  Loop i=0 to 36:
     │      XLlFifo_RxGetWord() → RxWord  ← READ PTR +1
     │      thisAxiType  = rxData[i]->axiType
     │      thisLocalType= rxData[i]->localType
     │      unitScale    = rxData[i]->scaleFactor
     │      check NaN if float
     │      convert type (float→double, u32→u32, etc.)
     │      scale (×1000)
     │      *rxData[i]->ptr = result  ← store in controller variable
     │
     ▼
     Back to while(1) — wait for next interrupt (125 µs later)
```

---

## Part 8: Key Concepts Summary

| Concept | Explanation |
|---------|-------------|
| **Interrupt** | Hardware way of saying "CPU, something happened!" — CPU stops current task, runs the registered handler, then returns |
| **GIC** | General Interrupt Controller — hardware dispatcher that knows which callback to call for which interrupt source |
| **FifoHandler** | First function called on interrupt — identifies interrupt type (RC/TC/Error) |
| **RC Flag** | "Receive Complete" — FIFO sets this when a full packet (TLAST=1) arrives |
| **READ PTR** | Advances by 1 every time `XLlFifo_RxGetWord()` is called — tracks how much FIFO was consumed |
| **axiData struct** | Per-word descriptor: destination pointer, local type, AXI type, scale factor |
| **Type punning** | Treating the same memory bytes as a different type (u32 → float without value conversion) |
| **volatile** | Tells compiler: "this can change at any time from an interrupt, never cache it in a CPU register" |
| **UNIT_SCALE = 1000** | RTDS sends kV/kA, controller uses V/A — multiply by 1000 on receive |
| **packBits()** | Packs 8 on/off signals into 1 integer using bit positions (bit 0, bit 1, ... bit 7) |
| **.elf file** | Compiled binary of all C files — built by Vitis IDE, loaded to Kria RAM via JTAG |
| **CHIL** | Controller Hardware-In-the-Loop — real controller tested against simulated power grid |
| **RX=37, TX=23** | RTDS sends many inputs (sensors + commands); Kria sends only controller outputs |
| **PEBB_Control_step()** | Auto-generated PLECS/Simulink model — executes one control timestep |
| **PL vs PS** | PL = FPGA logic (Aurora, FIFO — hardware). PS = Processor (A53 CPU — software) |
| **Memory-mapped registers** | CPU accesses FPGA hardware by reading/writing specific memory addresses |

---

## Part 9: Q&A — All Questions Answered

---

### Q1: Where do code files (main.c, Axi_IO.c) live? How do they get into memory?

**Answer:**

They live first on your development PC, then get compiled into a single `.elf` binary:

```
Windows PC:
  src/main.c + src/Axi_IO.c + src/UART.c + src/codegen.h
         │
         │  Vitis IDE → Build → aarch64-none-elf-gcc cross-compiler
         ▼
  Debug/xllfifo_interrupt_example_1.elf  (666 KB binary)
         │
         │  JTAG cable (or SD card boot)
         ▼
  Kria Board RAM ← .elf loaded here
         │
         ▼
  A53 CPU → starts executing main() instruction by instruction
```

The `.c` files themselves never "run" — they become machine code in the `.elf`, which the CPU executes directly from RAM. The files are not stored on a filesystem during operation.

Aurora IP and AXI FIFO are **hardware** — they are configured when the FPGA bitstream is programmed. They are not software files at all.

---

### Q2: Who recognizes the interrupt signal? What is the flow?

**Answer — the complete interrupt chain:**

```
HARDWARE (PL)                        SOFTWARE (PS)

AXI FIFO
  │ Word 37 (TLAST=1) arrives
  │ RC flag set in ISR register
  │
  │── electric signal (wire HIGH) ─────────────►  GIC
                                                   (General Interrupt Controller)
                                                    │
                                                    │ "FIFO interrupt received!
                                                    │  Registered handler: FifoHandler
                                                    │  Notify: A53 CPU"
                                                    │
                                                    ▼
                                                   A53 CPU
                                                    │
                                                    │ "Pause current task (while loop)
                                                    │  Save state (registers, PC)
                                                    │  Jump to registered handler"
                                                    │
                                                    ▼
                                               FifoHandler()  ← in main.c
```

- **FIFO hardware (PL):** detects TLAST, sets RC flag, asserts interrupt line
- **GIC (hardware on PS side):** receives the line, looks up the registered callback, notifies CPU
- **A53 CPU:** receives interrupt exception, saves state, jumps to `FifoHandler()`
- **FifoHandler (software):** identifies interrupt type, calls `FifoRecvHandler()`

---

### Q3: What exactly does `runControlCycle()` do?

**Simple story:** RTDS sent 37 words. Controller read them. Now:

1. Is `run=1`? (RTDS says to run?)
2. Is the packet valid 37 words? (not glitched?)
3. If yes → run `PEBB_Control_step()` once (one timestep of the controller)
4. Pack controller outputs → TX FIFO → send 23 words back to RTDS
5. Reset `Rx_Done = Tx_Done = 0` → ready for next cycle

The 3 conditions to actually run the controller: `ctrlOn=1 AND run=1 AND ReceiveLength==37`.

`PEBB_Control_step()` is auto-generated code from PLECS/Simulink. It reads from `PEBB_Control_U` (inputs) and writes to `PEBB_Control_Y` (outputs). We populate `PEBB_Control_U` via `axiReceive()`, then read `PEBB_Control_Y` via `axiSend()`.

---

### Q4: How does data convert and where does it get stored? (End-to-end for Word 2)

**Full pipeline for Word 2 (i_high — Phase A current):**

```
STEP 1 — RTDS sends 1.5 kA:
  IEEE 754 float bytes: 0x3FC00000
  This raw 32-bit word is stored in FIFO

STEP 2 — axiReceive() processes it:
  RxWord = XLlFifo_RxGetWord() → 0x3FC00000 (raw u32)

  rxData[1] = &rxData2:
    .ptr         = &PEBB_Control_U.feedback[0]  ← destination
    .axiType     = float_                        ← wire format
    .localType   = double_                       ← storage format
    .scaleFactor = 1000                          ← kA → A

  RxValue = *(float*)&RxWord     → 1.5f   (same bytes, read as float)
  *rxPtr  = (double)1.5f * 1000  → 1500.0 (Amperes)

STEP 3 — PEBB_Control_U.feedback[0] = 1500.0
  PEBB_Control_step() reads feedback[0]
  "Current = 1500A, Reference = 2000A → error = 500A → increase duty cycle"
  PEBB_Control_Y.switch_1[0] = 0.75

STEP 4 — axiSend() sends 0.75 back:
  float txWord = (float)0.75 * (1/1000) [scale back to kA range]
  XLlFifo_TxPutWord() → TX FIFO → Aurora → Fiber → RTDS
```

One-liner summary:
```
FIFO raw bytes → reinterpret as float → scale ×1000 → store as double
→ PEBB_Control_step() reads it → computes duty cycle → axiSend() sends back to RTDS
```

---

### Q5: Why are RX=37 words and TX=23 words different?

They are two separate packets with different purposes:

| Direction | Words | Contains |
|-----------|-------|----------|
| RTDS → Kria (RX) | **37 words** | 3-phase currents (A,B,C), 3-phase voltages, DC voltages, power commands (P, Q, V), contactor commands, power limits, ForceFault, run flag, sequence number, dummy/spare words |
| Kria → RTDS (TX) | **23 words** | 3-phase duty cycles, 8 binary flags (packed in 1 word), state (current + next), error flag, counter, sequence number, dummy/spare words |

**Why more RX than TX?** RTDS is simulating a complete power system — many sensor readings plus command inputs. Kria is only a controller — it outputs only switching decisions and status flags.

```c
// Axi_IO.h
#define RECEIVE_LENGTH  37   // RTDS → Kria
#define TRANSMIT_LENGTH 23   // Kria → RTDS
```

These values **must match exactly** on both RTDS and Kria. RTDS must be configured with 37 TX words and 23 RX words in its Aurora I/O configuration list.

---

### Q6: Does RTDS send all data at once? Is it a file?

**Answer:** RTDS sends one **complete packet** per simulation step — not a file, but a **snapshot**.

Every ~125 µs (8 kHz), RTDS completes one simulation timestep and calculates:
- New voltages, currents, states = 37 numbers

These 37 numbers are packed together into **one packet = 37 × 4 = 148 bytes** and sent as a single unit.

```
RTDS simulation timeline:
  t=0:      Step 1 completes → packet 1 sent [v=11.0kV, i=1.5kA, run=1, ...]
  t=125µs:  Step 2 completes → packet 2 sent [v=11.0kV, i=1.52kA, ...]
  t=250µs:  Step 3 completes → packet 3 sent [...]
  ...
```

RTDS does **not buffer** packets. It is a hard real-time system — one step, one packet, always.

---

### Q7: How much data does Aurora convert at a time?

**Answer:** Aurora IP converts data **serially to parallel**, one word (32-bit) per clock cycle. But from your software's perspective:

- Aurora receives the entire 37-word packet as serial bits on the fiber
- It converts and forwards each word to the AXI FIFO one by one
- FIFO accumulates all 37 words
- When TLAST arrives → interrupt fires

You never interact with Aurora directly — it's fully transparent hardware between RTDS and the FIFO.

---

### Q8: What is AXI Stream and why does it matter?

**Answer:** AXI-Stream is the internal communication protocol inside the FPGA between Aurora IP and AXI FIFO.

Think of it as a **one-way highway** inside the chip:

```
Aurora IP  ──────────────────────────────────────►  AXI FIFO

Every clock cycle one "beat" travels:
  Beat 1:  TDATA=word1   TVALID=1  TLAST=0   ← data valid, not last
  Beat 2:  TDATA=word2   TVALID=1  TLAST=0
  ...
  Beat 37: TDATA=word37  TVALID=1  TLAST=1   ← data valid AND this is the last!
```

The `TLAST=1` on beat 37 is what causes the FIFO to:
1. Mark the packet as "complete"
2. Set the RC interrupt flag
3. Signal the CPU

Without AXI Stream, Aurora and FIFO would not know how to communicate inside the FPGA.

---

### Q9: How large is the FIFO? How much data does it hold?

**Answer:**

| Attribute | Value |
|-----------|-------|
| 1 slot | 1 word = 32 bits = 4 bytes |
| Typical FIFO depth | 512–4096 words |
| At 512 depth | 512 × 4 = 2 KB total capacity |
| Our 1 packet | 37 words = 148 bytes |
| Packets that fit | ~13 at 512 depth (but we process 1 at a time) |

In practice, the FIFO always has at most **1 packet (37 words)** because we process and drain it before the next packet arrives (125 µs later at 8 kHz).

The actual depth configured for our project can be verified in `xparameters.h` (auto-generated by Vivado/Vitis during hardware export).

---

### Q10: When exactly does the interrupt fire?

**Answer:** The interrupt fires precisely when the **last word of a complete packet enters the FIFO** — i.e., when the AXI Stream beat with `TLAST=1` is stored.

```
Aurora → FIFO: Beat 1  (TLAST=0)  → word 1 stored
Aurora → FIFO: Beat 2  (TLAST=0)  → word 2 stored
Aurora → FIFO: Beat 3  (TLAST=0)  → word 3 stored
...
Aurora → FIFO: Beat 37 (TLAST=1)  → word 37 stored ← INTERRUPT FIRES HERE!
```

Timeline:
```
Time ──────────────────────────────────────────────────────►

RTDS transmits:   [pkt 1: 37 words]       [pkt 2: 37 words]
                              │                         │
                              ▼                         ▼
FIFO interrupt:           [INT!]                    [INT!]
                              │                         │
A53 CPU:     [idle]──►[FifoRecvHandler]  [idle]──►[FifoRecvHandler]
                       [reads 37 words]             [reads 37 words]
                       [runs controller]            [runs controller]
                       [sends TX data]              [sends TX data]
                       [goes idle again]            [goes idle again]
```

The CPU is idle in `while(1)` between packets, then wakes up for ~microseconds to process each packet.

---

### Q11: What does a "complete packet" mean in AXI Stream terms?

**Answer:** In the AXI4-Stream protocol, any number of words can form a "packet" — the sender just needs to assert `TLAST=1` on the final beat.

For our RTDS system specifically:
- Complete = `TLAST=1` received AND `ReceiveLength == 37`
- Incomplete = `TLAST=1` but `ReceiveLength != 37` (glitch, partial transmission)

```c
// axiReceive() validates both conditions:
ReceiveLength = (XLlFifo_iRxGetLen(InstancePtr)) / WORD_SIZE;

if (ReceiveLength != RECEIVE_LENGTH) {
    // Wrong count — drain FIFO, log error, skip processing
} else {
    // Correct count — process all 37 words
    for (i = 0; i < RECEIVE_LENGTH; i++) {
        RxWord = XLlFifo_RxGetWord(InstancePtr);
        // ... convert, scale, store
    }
}
```

The sequence number (Word 37, `rxDataSeq`) provides a second layer of validation:
```c
// Axi_IO.c line 335
axiData rxDataSeq = {
    .ptr = (void*) &seq,
    .axiType = s32_,
    ...
};
// After axiReceive(), if seq != seq_prev+1 → "Warning: missing packet"
```

---

*End of document. See `kr260_aurora_fifo_pattern_generator.md` for the pattern generator explanation (separate document).*
