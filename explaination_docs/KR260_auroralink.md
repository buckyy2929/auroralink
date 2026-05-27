# KR260 Aurora + FIFO + Pattern Generator — Complete Setup Guide

> **Goal:** Test our Aurora link end-to-end on the KR260 board WITHOUT needing the actual RTDS simulator. We replace RTDS with a "Pattern Generator" inside the FPGA that fakes the data RTDS would send.

---

## Table of Contents

1. [Big Picture — What Are We Actually Building?](#part-1)
2. [The Train Station Refresher](#part-2)
3. [Block-by-Block Architecture (with train analogy)](#part-3)
4. [What is `m_axis` in the .v file? — Line by Line](#part-4)
5. [Counter / Pattern Generator Design — Full Verilog](#part-5)
6. [Does the Counter Output Match Aurora Format?](#part-6)
7. [TX Pin Connections — Where Does Aurora TX Physically Go?](#part-7)
8. [Loopback vs Real Fiber — Which One and When?](#part-8)
9. [Clock and Reset — Detailed Explanation](#part-9)
10. [Why Do We Need ILA? Is Debug Required?](#part-10)
11. [Buffer/FIFO Handling — Race Conditions and Backpressure](#part-11)
12. [Complete Closed-Loop Cycle — Counter → Controller → Switches → Back](#part-12)
13. [Step-by-Step Vivado Implementation Guide](#part-13)
14. [Bring-Up and Testing on KR260 (from scratch)](#part-14)
15. [Q&A — Every Question You Will Face](#part-15)

---

<a id="part-1"></a>
## Part 1: Big Picture — What Are We Actually Building?

### 1.1 The Real System (production)

```
RTDS Simulator (PC)  ──fiber──►  Aurora RX  ──►  AXI FIFO  ──►  A53 CPU (runs main.c, Axi_IO.c)
                                                                       │
                                                                       │ controller computes
                                                                       │ duty cycles
                                                                       ▼
RTDS Simulator (PC)  ◄──fiber──  Aurora TX  ◄──  AXI FIFO  ◄──  A53 CPU
```

In production, **RTDS** is a real-time power-system simulator that streams a frame of **37 × 32-bit words** (= 148 bytes; see `RECEIVE_LENGTH 37` in `Axi_IO.h`) over fiber at the RTDS simulation step rate. The Kria board receives, runs the controller, and sends **23 words** back (`TRANSMIT_LENGTH 23`).

> **About the frame rate.** Every text on the bench says "every 125 µs / 8 kHz" — but this number is set by the **RTDS model**, not by Aurora itself, and we have not yet located the authoritative RTDS docs section that pins it down (see `docs/milestone2/M2-A_auroralink_findings.md` Known Gaps). Aurora delivers a frame whenever the partner sends one; the period is empirically the inter-RC-interrupt time on the PS once the board is alive. Treat any specific µs figure in this doc as "expected order-of-magnitude only" until measured on hardware.

### 1.2 The Lab/Bench Test System (this document)

We don't have RTDS on our desk. So we **fake it inside the FPGA** with a "Pattern Generator":

```
Pattern Generator (Verilog)  ──►  TX FIFO  ──►  Aurora TX  ─┐
                                                            │ (loopback —
                                                            │  TX wires
                                                            │  connect to
                                                            │  RX wires
                                                            │  internally)
A53 CPU (controller)  ◄──  RX FIFO  ◄──  Aurora RX  ◄───────┘
            │
            ▼
        Computes duty cycles, sends 23 words back via own TX path
```

**The pattern generator's job:** pretend to be RTDS. Generate 37 valid words on a schedule, push them through Aurora, get them back via loopback, and let our existing software process them.

### 1.3 Why Do This?

1. **No RTDS available** — RTDS hardware is expensive and shared. We can develop on Kria alone.
2. **Verify the link works** — Before connecting real RTDS, prove the Aurora + FIFO + interrupt + software chain works end-to-end.
3. **Debug at our own pace** — RTDS sends data at a fixed model rate (expected order of magnitude: ~kHz, exact figure pending — see RTDS doc gap above); pattern generator can be slowed to 1 Hz, paused, single-stepped.
4. **Reproducible test data** — Pattern generator sends predictable values (e.g., counter 1, 2, 3, ...) so we can verify what we receive.

### 1.4 Scope of THIS document — what is and is not validated yet

This guide is the *design + bring-up plan* for the Aurora link on KR260. The author **does not yet have a Kria board on the desk**. To set honest expectations for the reviewer:

| Status | Item | Where evidence lives |
|---|---|---|
| ✅ Verified from source | Aurora IP variant, line rate, refclk, GT channel, board target | `top.xsa/design_1.hwh` (extracted from the supplied CHIL bitstream); see `docs/milestone2/M2-A_auroralink_findings.md` §2 |
| ✅ Verified from source | PS-side packet sizes (37 RX / 23 TX words, 32-bit each) | `RTDS_Aurora_Link/.../xllfifo_interrupt_example_1/src/Axi_IO.h` lines 21–23 |
| ✅ Verified from source | Frame-boundary signalling path (PL push → FIFO → `pl_ps_irq0` → GIC → FifoHandler → axiReceive) | `design_1.hwh`, `main.c`, `Axi_IO.c`; see M2-A §3–§4 |
| ⏳ Designed, not yet executed | Standalone Aurora loopback bitstream (`scripts/vivado/build_aurora_loopback.tcl`) | TCL committed; never run synthesis on this machine (no Vivado installed) |
| ⏳ Designed, not yet executed | All "Phase A–G" bring-up steps in §14 | Same — they are the script the engineer with the board will execute |
| ❌ Not measured | Actual frame rate ("125 µs / 8 kHz") | Not sourced from RTDS docs in this repo; pending RTDS manual section citation |
| ❌ Not measured | `channel_up` assert latency, RC IRQ latency, bit-equality on loopback | Pending board |
| ❌ Not measured | Pin-level signal integrity, GTH PLL lock | Pending board |

**What this means for the reviewer:**

- Every **architectural / configuration** claim in this doc is sourced to a file path (the `top.xsa` hardware handoff or the CHIL reference source). If a number does not have a source nearby, it is hedged as "expected" or "assumed".
- Every **measurement / observed-on-silicon** claim is fenced behind §14 ("Bring-Up and Testing on KR260") and is **explicitly listed as pending board access**, not silently asserted as done.
- The hand-wavy "8 kHz / 125 µs RTDS rate" is the single biggest unsourced number in the codebase. It is flagged in §1.1 above, in §5.5 (`INTER_PKT_GAP`), and in `docs/milestone2/M2-A_auroralink_findings.md` Known Gaps. Resolution requires either an RTDS docs citation or a measurement on the bring-up bench.

Read the rest of this document with that lens.

<a id="part-2"></a>
## Part 2: The Train Station Refresher

Throughout this guide, we use a train station analogy. Memorize this once:

| Real Component | Train Analogy |
|----------------|---------------|
| RTDS / Pattern Generator | **Train Driver** at the origin station — fills 37 wagons with cargo, schedules departure |
| Pattern Generator output (AXI Stream) | **Train pulling out of the depot** — wagons in a row |
| TX FIFO | **Cargo loading platform** — wagons wait here briefly while station signals are set |
| Aurora TX | **The locomotive engine** — encodes wagons into fiber-optic light pulses, pushes onto track |
| Fiber optic cable / Loopback | **Railway track** — single rail, one direction |
| Aurora RX | **Receiving locomotive** — decodes light pulses back into wagons |
| RX FIFO | **Arrival platform** — wagons wait here for the station master to inspect |
| Interrupt (RC flag) | **Platform bell** — rings when last wagon arrives (TLAST=1) |
| A53 CPU | **Station Master** — only acts when bell rings, processes wagon contents |
| ILA (debug) | **Security cameras** at every checkpoint — record what happened, replay later |

> **Train Tip:** Every step in this document maps to either "filling a wagon", "pulling onto the track", "arriving at platform", or "station master action". If you get lost, come back here.

<a id="part-3"></a>
## Part 3: Block-by-Block Architecture

### 3.1 Full Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                            KR260 BOARD — FPGA SIDE (PL)                      │
│                                                                              │
│  ┌────────────────┐                                                          │
│  │  Pattern       │  M_AXIS                                                  │
│  │  Generator     │ ─────────►┌─────────────┐                                │
│  │  (counter.v)   │           │  TX AXIS    │ M_AXIS                         │
│  │                │           │  Data FIFO  │ ───────►┌──────────────────┐   │
│  │  Generates 37  │           │  (Xilinx)   │         │  Aurora 8B/10B  │   │
│  │  word packets  │           └─────────────┘         │  (TX side)       │   │
│  └────────────────┘                                   │                  │   │
│                                                       │  GTH transceiver │   │
│        ┌──────────────────────────────────────────────┤  TX pin          │   │
│        │  Internal PMA loopback OR external fiber    │                  │   │
│        ▼                                              │  RX pin          │   │
│  ┌────────────────┐                                   │                  │   │
│  │  Aurora 8B/10B│                                   │  8B/10B encoder  │   │
│  │  (RX side)     │                                   │  CRC/scrambler   │   │
│  │                │                                   └──────────────────┘   │
│  │  Decodes 8b10b │  M_AXI_RX                                                │
│  │  back to AXI   │ ─────────►┌─────────────┐                                │
│  │  Stream        │           │  AXI4-Stream│  AXI-Lite (CPU control)        │
│  └────────────────┘           │  FIFO       │ ◄═════════════════════════╗    │
│                               │  (PG080)    │                           ║    │
│                               │  RX side    │                           ║    │
│                               │             │ ── interrupt ─────────╗   ║    │
│                               └─────────────┘                       ║   ║    │
│                                                                     ║   ║    │
│  ┌──── ILA ──────────────────────────────────────────────────────┐  ║   ║    │
│  │  Snoop on TDATA, TVALID, TREADY, TLAST, channel_up           │  ║   ║    │
│  └───────────────────────────────────────────────────────────────┘  ║   ║    │
│                                                                     ║   ║    │
│ ════════════════════════════════════════════════════════════════════╬═══╬════│
│                            PROCESSING SYSTEM (PS)                   ║   ║    │
│                                                                     ║   ║    │
│  ┌────────────────────────────────────────────────────────────┐    ║   ║    │
│  │  A53 CPU running our Vitis bare-metal application:        │    ║   ║    │
│  │   - main.c (interrupt setup, FifoHandler)                 │    ║   ║    │
│  │   - Axi_IO.c (axiReceive, axiSend)                        │    ║   ║    │
│  │   - PEBB_Control_step() (auto-generated PLECS controller) │    ║   ║    │
│  │                                                            │ ◄══╝   ║    │
│  │  GIC receives interrupt ─► FifoHandler ─► axiReceive ─►   │        ║    │
│  │   reads 37 words from RX FIFO via memory-mapped registers │ ═══════╝    │
│  └────────────────────────────────────────────────────────────┘             │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 What Each Block Does (Train Analogy)

#### Block 1 — Pattern Generator (`axis_pattern_generator.v`)
**Train role:** The train driver who fills wagons.

- We write this in Verilog (it doesn't exist as a Xilinx IP — we create it).
- Every cycle, it produces one 32-bit word on `m_axis_tdata`.
- After 37 words, it asserts `m_axis_tlast=1` to signal "end of train".
- Then it waits some configurable time (e.g., a counter or timer) before sending the next 37-word train.

#### Block 2 — TX AXIS Data FIFO (Xilinx IP)
**Train role:** Loading platform that holds wagons briefly before the locomotive picks them up.

- This is a small buffer between the pattern generator and Aurora.
- Why we need it: Aurora may not always be "ready" (`tready=0`) — for example during initialization. The FIFO absorbs this so the pattern generator can keep producing.
- Xilinx IP name: **AXI4-Stream Data FIFO** (note: this is the small simple FIFO, not the same as the big PG080 RX FIFO with interrupts).
- Recommended depth: 64 to 256 entries.

#### Block 3 — Aurora 8B/10B IP (TX side)
**Train role:** The locomotive that pulls the wagons onto the railway track.

- Takes AXI Stream input (`s_axi_tx`).
- Encodes each byte using **8B/10B line coding** (sourced from `top.xsa/design_1.hwh`: `aurora_8b10b` v11.1, PG046, 1 lane, 2 Gbps line rate). 2 Gbps × 8/10 ÷ 32-bit lane = 50 MHz user-side AXI-Stream clock.
- Outputs **serial differential pair** (`gtX_txp` and `gtX_txn`) — these go to the GTH transceiver pins.
- Also handles handshaking (`channel_up` signal goes high once link is established).

#### Block 4 — Loopback / Fiber Cable
**Train role:** The railway track itself.

Two options here (covered in detail in [Part 8](#part-8)):

1. **Internal PMA loopback** — Aurora's TX pins are wired internally back to RX pins inside the FPGA's serial transceiver. No external cable needed. Ideal for first bring-up.
2. **External fiber loopback** — A real fiber cable plugs the TX SFP into the RX SFP on the same board. Tests the actual physical layer.
3. **External link to RTDS** — Real production setup.

#### Block 5 — Aurora 8B/10B IP (RX side)
**Train role:** The receiving locomotive that hauls wagons back to the destination platform.

- Decodes the 8b/10b line-encoded symbols back to plain 32-bit words.
- Outputs AXI Stream (`m_axi_rx`).
- Asserts `m_axi_rx_tlast` when packet ends.

> Note: In our setup, **the same Aurora IP block has both TX and RX** — they are two halves of one IP, not separate IPs.

#### Block 6 — RX FIFO (AXI4-Stream FIFO, PG080)
**Train role:** Arrival platform with a station bell.

- This is the **big, smart FIFO** — Xilinx PG080 — different from the small Data FIFO on TX side.
- Has memory-mapped registers (AXI-Lite) the CPU reads via `XLlFifo_*` driver functions in `Axi_IO.c`.
- Has an **interrupt output** that goes to the GIC (General Interrupt Controller) on the PS side.
- When TLAST is received, it asserts the **RC (Receive Complete)** interrupt.
- Software calls `XLlFifo_RxGetWord()` 37 times to drain it.

#### Block 7 — ILA (Integrated Logic Analyzer)
**Train role:** Security cameras at multiple checkpoints.

- Xilinx debug IP. Records signal values in real time inside the FPGA, into BRAM.
- We connect ("probe") it to all the AXI Stream signals.
- After running, we open Vivado Hardware Manager and view a waveform of what actually happened on the wires.
- More in [Part 10](#part-10).

#### Block 8 — A53 CPU (PS)
**Train role:** Station master who acts on the bell.

- Runs our existing software (`main.c` + `Axi_IO.c`) — unchanged from production.
- Receives interrupt from RX FIFO → reads 37 words → runs controller → sends 23 words via TX path.

<a id="part-4"></a>
## Part 4: What is `m_axis` in the .v file? — Line by Line

### 4.1 What is AXI Stream? (Quick Recap)

AXI Stream is a simple unidirectional protocol used inside FPGAs to move data from one block to another. It is the "highway" between blocks.

It has **5 main signals** (we call them the "AXI Stream signals"):

| Signal | Direction | Meaning |
|--------|-----------|---------|
| `tdata[31:0]` | output | The actual 32-bit data word |
| `tvalid` | output | "I have valid data right now" — sender says |
| `tready` | input | "I can accept it right now" — receiver says |
| `tlast` | output | "This is the last word of the packet" |
| `aclk` | input | The clock — everything is synchronous to this |
| `aresetn` | input | Active-low reset (`n` = "not", i.e., 0 means reset) |

A transfer happens **only on a clock edge where `tvalid=1` AND `tready=1` simultaneously**. This is called a "handshake".

### 4.2 What does `m_axis` mean? — The Naming Convention

`m_axis` = **M**aster **AXI S**tream port.

In AXI, every connection has two sides:
- **Master** = the side that produces data (sends)
- **Slave** = the side that consumes data (receives)

Naming conventions in Xilinx tools:
- Signals starting with `m_axis_*` → this block is the MASTER (it outputs data).
- Signals starting with `s_axis_*` → this block is the SLAVE (it receives data).

When you package your custom RTL as an IP in Vivado, the tool **automatically detects** the AXI Stream interface based on these naming patterns. That's why we MUST use these exact names.

### 4.3 The Pattern Generator's `.v` File — Skeleton

```verilog
module axis_pattern_generator #(
    parameter PACKET_LENGTH = 37,        // 37 words per train
    parameter INTER_PKT_GAP = 32'd100000 // wait this many clocks between trains
)(
    input  wire        aclk,             // clock — same as Aurora user clock
    input  wire        aresetn,          // active-low reset

    // AXI Stream MASTER port — this block PRODUCES data
    output wire [31:0] m_axis_tdata,     // 32-bit data
    output wire        m_axis_tvalid,    // "data is valid"
    input  wire        m_axis_tready,    // "downstream is ready" (FIFO accepts)
    output wire        m_axis_tlast      // "last word of packet"
);
    // ... internal logic ...
endmodule
```

**Train analogy for each signal:**

- `m_axis_tdata` = the cargo inside the wagon (what you send)
- `m_axis_tvalid` = the conductor waving a green flag ("ready to depart!")
- `m_axis_tready` = the next station replying "yes, send it" (handshake)
- `m_axis_tlast` = the LAST WAGON has a red flag attached ("end of this train")
- A wagon moves only when **both** flags are up: green from sender + green-back from receiver.

### 4.4 What Vivado Does With These Names

When you click **Tools → Create and Package New IP**:

1. Vivado scans your `.v` file's port list.
2. It sees `m_axis_tdata`, `m_axis_tvalid`, `m_axis_tready`, `m_axis_tlast`.
3. It auto-groups them into one **AXI4-Stream Master interface** named `m_axis`.
4. Now in the Block Design GUI, this whole bundle appears as a single connection point — you just drag a wire from your IP's `m_axis` pin to the FIFO's `s_axis` pin.

**If you misspell the names** (e.g., `tdat` instead of `tdata`, or `m_axi_tdata` without the `s`), Vivado will NOT recognize them as AXI Stream → you'll have to wire each signal individually, which is messy and error-prone.

### 4.5 The "s_axis" Side (Why It Matters)

The TX FIFO has a **slave** AXI Stream input port, named `s_axis`.

```
Pattern Generator                 TX FIFO
    │                                │
    │  m_axis_tdata    ───────►      s_axis_tdata
    │  m_axis_tvalid   ───────►      s_axis_tvalid
    │  m_axis_tready   ◄───────      s_axis_tready
    │  m_axis_tlast    ───────►      s_axis_tlast
```

> Master output → Slave input. Always.

<a id="part-5"></a>
## Part 5: Counter / Pattern Generator Design — Full Verilog

### 5.1 Why a "Counter" Pattern Generator?

The simplest pattern generator is a **counter**: every word it sends is just an incrementing integer (1, 2, 3, ...). This makes it trivial to verify correctness:
- If we send 1, 2, 3, ..., 37 and receive back 1, 2, 3, ..., 37 → link works.
- If we receive 1, 2, 7, 4, ... → link is corrupted.
- If we receive 1, 2, 3 then nothing → link broke after word 3.

But **for our specific system**, a pure counter is NOT enough. The software (`Axi_IO.c`) expects:
- Word 35 = `run` flag (must be 1, otherwise controller doesn't run)
- Most words 2–34 = IEEE 754 floats (a counter value would be interpreted as garbage float, possibly NaN → CHIL_error fires)
- Word 37 = sequence number (1 to 1000, increments)

**So we need a "smart counter" pattern generator** that produces values our software accepts.

### 5.2 Reading `Axi_IO.c` — What Format Must We Produce?

From `Axi_IO.h`:
```c
#define WORD_SIZE 4              // 4 bytes per word
#define RECEIVE_LENGTH 37        // exactly 37 words per packet
#define UNIT_SCALE 1000          // RTDS sends kV/kA, code expects V/A
#define SEQ_MAX 1000             // sequence number max value
```

From `Axi_IO.c` — the 37-word RX layout:

| Word # | Variable | `axiType` | `localType` | Scale | What value to send |
|--------|----------|-----------|-------------|-------|--------------------|
| 1  | `key` | s32 | s32 | 1 | any int (e.g., 0xCAFE0001) |
| 2  | `feedback[0]` (i_high A) | float | double | 1000 | float, e.g., 1.5 (= 1.5 kA → 1500 A) |
| 3  | `feedback[4]` (vhigh B) | float | double | 1000 | float, e.g., 11.0 |
| 4  | `feedback[3]` (vhigh A) | float | double | 1000 | float, e.g., 11.0 |
| 5  | `feedback[7]` (v_dcc) | float | double | 1000 | float, e.g., 12.0 |
| 6  | `feedback[6]` (i_low) | float | double | 1000 | float, e.g., 0.5 |
| 7  | dummy | float | double | 1000 | any float (e.g., 0.0) |
| 8  | `PEBB_NEXT_STATE_CMD` | float | double | 1 | any float |
| 9  | `P_CMD` | float | double | 1000 | float, e.g., 5.0 |
| 10 | `LOWSIDE_CONTACTOR_CMD` | float | double | 1 | float, e.g., 1.0 |
| 11 | `PEBB_POWER_LOWER_LIMIT` | float | double | 1e6 | float, e.g., 0.0 |
| 12 | `PEBB_POWER_UPPER_LIMIT` | float | double | 1e6 | float, e.g., 10.0 |
| 13 | dummy | float | double | 1000 | any float (0.0 OK) |
| 14 | dummy | float | double | 1000 | any float (0.0 OK) |
| 15 | `ctr` (substep counter) | s32 | s32 | 1 | int counter |
| 16-19 | dummies | float | double | 1000 | any non-NaN float |
| 20 | `V_CMD` | float | double | 1000 | float, e.g., 11.0 |
| 21 | `ForceFault` | float | double | 1 | float, e.g., 0.0 (no fault) |
| 22-25 | dummies | float | double | 1 | any non-NaN float |
| 26 | `EN_GRID_SUPPORT` | float | double | 1 | float, e.g., 0.0 |
| 27 | `Q_CMD` | float | double | 1000 | float, e.g., 0.0 |
| 28 | `Q_LIMIT` | float | double | 1e6 | float, e.g., 5.0 |
| 29 | `feedback[1]` (i_high B) | float | double | 1000 | float |
| 30 | `feedback[2]` (i_high C) | float | double | 1000 | float |
| 31 | `feedback[5]` (v_high C) | float | double | 1000 | float |
| 32-34 | `feedback[8..10]` (v_stator) | float | double | 1000 | float |
| 35 | `run` | s32 | s32 | 1 | **MUST BE 1** to run controller |
| 36 | `RXI_chil` | s32 | s32 | 1 | int (0 or 1) |
| 37 | `seq` (sequence number) | s32 | s32 | 1 | counter 1..1000, increments |

> **CRITICAL:** float words must be valid IEEE 754 floats — NOT NaN. A NaN sets `CHIL_error |= 1` and prints an error in the UART log.

### 5.3 IEEE 754 Float Encoding — How to Send a Number

To send the float value `1.5` as 32 bits, you encode it in IEEE 754 single-precision:

| Bits | Field | Value for 1.5 |
|------|-------|---------------|
| [31] | sign | 0 (positive) |
| [30:23] | exponent (biased) | 127 (0x7F) |
| [22:0] | mantissa | 0x400000 |

So `1.5f` in hex = `0x3FC00000`.

**Common preset values you can hard-code in Verilog:**

| Value | IEEE 754 hex |
|-------|--------------|
| 0.0 | `32'h00000000` |
| 1.0 | `32'h3F800000` |
| 1.5 | `32'h3FC00000` |
| 2.0 | `32'h40000000` |
| 5.0 | `32'h40A00000` |
| 10.0 | `32'h41200000` |
| 11.0 | `32'h41300000` |
| 12.0 | `32'h41400000` |
| -1.0 | `32'hBF800000` |
| 100.0 | `32'h42C80000` |
| 1000.0 | `32'h447A0000` |

> **Important:** Do NOT try to convert floats inside Verilog — pre-compute them on paper or in Python (`struct.pack('<f', 1.5).hex()`) and hard-code the hex. Doing float math in Verilog requires DSP/floating-point IP, which is overkill for a test pattern.

### 5.4 Smart Counter Generator — Full Verilog

This generator sends a fixed packet of 37 valid words, increments the sequence number each packet, and uses an inter-packet gap.

```verilog
//-------------------------------------------------------------------------
// axis_pattern_generator.v
// 37-word AXI4-Stream pattern generator that mimics RTDS output.
// Produces valid IEEE-754 floats and integers in the format expected by
// Axi_IO.c on the A53 PS side.
//-------------------------------------------------------------------------
module axis_pattern_generator #(
    parameter integer PACKET_LENGTH = 37,
    parameter integer INTER_PKT_GAP = 32'd1_000_000  // ~10 ms at 100 MHz
)(
    input  wire        aclk,
    input  wire        aresetn,

    output reg  [31:0] m_axis_tdata,
    output reg         m_axis_tvalid,
    input  wire        m_axis_tready,
    output reg         m_axis_tlast
);

    // ROM holding 37 fixed payload values.
    // Index 0 = word 1, index 36 = word 37.
    // Words 1, 15, 35, 36, 37 are integers; everything else is IEEE-754 float.
    reg [31:0] payload [0:36];

    initial begin
        payload[0]  = 32'hCAFE0001;     // Word 1: key (s32)
        payload[1]  = 32'h3FC00000;     // Word 2: i_high     = 1.5 kA
        payload[2]  = 32'h41300000;     // Word 3: vhigh_B    = 11.0 kV
        payload[3]  = 32'h41300000;     // Word 4: vhigh_A    = 11.0 kV
        payload[4]  = 32'h41400000;     // Word 5: v_dcc      = 12.0 kV
        payload[5]  = 32'h3F000000;     // Word 6: i_low      = 0.5 kA
        payload[6]  = 32'h00000000;     // Word 7: dummy
        payload[7]  = 32'h00000000;     // Word 8: NEXT_STATE = 0.0
        payload[8]  = 32'h40A00000;     // Word 9: P_CMD      = 5.0 MW
        payload[9]  = 32'h3F800000;     // Word 10: LOWSIDE_CONTACTOR = 1.0
        payload[10] = 32'h00000000;     // Word 11: P_LOW     = 0.0
        payload[11] = 32'h41200000;     // Word 12: P_UP      = 10.0
        payload[12] = 32'h00000000;     // Word 13: dummy
        payload[13] = 32'h00000000;     // Word 14: dummy
        // Word 15 (ctr) is dynamic — see substep_counter below
        payload[15] = 32'h00000000;     // Word 16: dummy
        payload[16] = 32'h00000000;     // Word 17: dummy
        payload[17] = 32'h00000000;     // Word 18: dummy
        payload[18] = 32'h00000000;     // Word 19: dummy
        payload[19] = 32'h41300000;     // Word 20: V_CMD     = 11.0
        payload[20] = 32'h00000000;     // Word 21: ForceFault= 0.0 (no fault)
        payload[21] = 32'h00000000;     // Word 22: dummy
        payload[22] = 32'h00000000;     // Word 23: dummy
        payload[23] = 32'h00000000;     // Word 24: dummy
        payload[24] = 32'h00000000;     // Word 25: dummy
        payload[25] = 32'h00000000;     // Word 26: EN_GRID   = 0.0
        payload[26] = 32'h00000000;     // Word 27: Q_CMD     = 0.0
        payload[27] = 32'h40A00000;     // Word 28: Q_LIMIT   = 5.0
        payload[28] = 32'h3FC00000;     // Word 29: i_high_B  = 1.5
        payload[29] = 32'h3FC00000;     // Word 30: i_high_C  = 1.5
        payload[30] = 32'h41400000;     // Word 31: v_high_C  = 12.0
        payload[31] = 32'h41300000;     // Word 32: v_stator_A= 11.0
        payload[32] = 32'h41300000;     // Word 33: v_stator_B= 11.0
        payload[33] = 32'h41300000;     // Word 34: v_stator_C= 11.0
        payload[34] = 32'h00000001;     // Word 35: run = 1   (REQUIRED!)
        payload[35] = 32'h00000001;     // Word 36: RXI_chil = 1
        // Word 37 (seq) is dynamic — see seq_counter below
    end

    // FSM states
    localparam ST_IDLE  = 2'd0;
    localparam ST_SEND  = 2'd1;
    localparam ST_GAP   = 2'd2;

    reg [1:0]  state;
    reg [5:0]  word_idx;          // 0..36 (6 bits is enough)
    reg [31:0] gap_count;
    reg [31:0] substep_counter;   // for word 15 (ctr) — increments each packet
    reg [31:0] seq_counter;       // for word 37 (seq) — 1..1000 wraps

    always @(posedge aclk) begin
        if (!aresetn) begin
            state            <= ST_IDLE;
            word_idx         <= 6'd0;
            gap_count        <= 32'd0;
            substep_counter  <= 32'd0;
            seq_counter      <= 32'd1;     // start at 1
            m_axis_tdata     <= 32'd0;
            m_axis_tvalid    <= 1'b0;
            m_axis_tlast     <= 1'b0;
        end else begin
            case (state)
                //----------------------------------------------------------
                // IDLE: prepare to send a new packet
                //----------------------------------------------------------
                ST_IDLE: begin
                    word_idx      <= 6'd0;
                    m_axis_tvalid <= 1'b1;
                    m_axis_tlast  <= 1'b0;
                    m_axis_tdata  <= payload[0];   // word 1 = key
                    state         <= ST_SEND;
                end

                //----------------------------------------------------------
                // SEND: stream one word per accepted handshake
                //----------------------------------------------------------
                ST_SEND: begin
                    if (m_axis_tvalid && m_axis_tready) begin
                        // word just accepted — advance
                        if (word_idx == PACKET_LENGTH - 1) begin
                            // last word was just sent — go to gap
                            m_axis_tvalid <= 1'b0;
                            m_axis_tlast  <= 1'b0;
                            gap_count     <= 32'd0;
                            substep_counter <= substep_counter + 1;
                            seq_counter   <= (seq_counter == SEQ_MAX) ?
                                             32'd1 : seq_counter + 1;
                            state         <= ST_GAP;
                        end else begin
                            word_idx <= word_idx + 1;
                            // pick next word's data
                            case (word_idx + 1)  // word_idx will be this value next cycle
                                6'd14: m_axis_tdata <= substep_counter;          // word 15 = ctr
                                6'd36: m_axis_tdata <= seq_counter;              // word 37 = seq
                                default: m_axis_tdata <= payload[word_idx + 1]; // ROM
                            endcase
                            // assert TLAST on the LAST word
                            m_axis_tlast <= (word_idx + 1 == PACKET_LENGTH - 1);
                        end
                    end
                    // if !tready, hold tvalid/tdata/tlast steady (AXIS rule)
                end

                //----------------------------------------------------------
                // GAP: wait INTER_PKT_GAP cycles before next packet
                //----------------------------------------------------------
                ST_GAP: begin
                    if (gap_count == INTER_PKT_GAP - 1) begin
                        state <= ST_IDLE;
                    end else begin
                        gap_count <= gap_count + 1;
                    end
                end

                default: state <= ST_IDLE;
            endcase
        end
    end

    // Helper localparam
    localparam SEQ_MAX = 32'd1000;

endmodule
```

**Train story for this module:**

1. **IDLE state** — engine sits at the depot. Driver walks in, picks up wagon 1, signals "ready to depart" (`tvalid=1`).
2. **SEND state** — for each cycle where the next station says "send it" (`tready=1`), one wagon moves out. After wagon 36 sends, wagon 37 has the red flag (`tlast=1`).
3. **GAP state** — engine returns to depot, waits a fixed number of clocks (e.g., 10 ms worth at 100 MHz = 1,000,000 cycles), so the receiving station has time to process before the next train comes.
4. **Sequence counter** — each train gets a new ID (1, 2, ..., 1000, then wraps).

### 5.5 Choosing INTER_PKT_GAP

RTDS sends one packet at its model step rate. The 125 µs / 8 kHz figure commonly quoted in the codebase is **not** sourced from an RTDS docs citation in this repo — treat it as a target order of magnitude only (see `docs/milestone2/M2-A_auroralink_findings.md` Known Gaps). The table below lets the pattern generator span the plausible range from "one packet per second" (easy to debug) up to "RTDS-realistic":

| Goal | Period @ 100 MHz pl_clk0 | INTER_PKT_GAP value |
|------|--------------------------|---------------------|
| RTDS-realistic (assumed 8 kHz pending docs citation) | 125 µs | 12,500 |
| 1 packet per ms (slower) | 1 ms | 100,000 |
| 1 packet per 10 ms | 10 ms | 1,000,000 |
| 1 packet per second (very slow, easy to single-step) | 1 s | 100,000,000 |

> Tip: Start very slow (1 packet/sec). Verify everything works. Then speed up. Once you have the actual RTDS partner connected, measure the inter-RC-interrupt time on the PS and update this table with the measured value.

### 5.6 Why This Counter Output IS Valid for Our Software

| Software Check | How We Pass It |
|----------------|----------------|
| `ReceiveLength == 37` | We send exactly 37 words and assert `tlast` on word 37 only. |
| No NaN floats | All hard-coded floats are valid IEEE 754 (0.0, 1.0, 5.0, etc.). |
| `run == 1` | Word 35 hard-coded to `32'h00000001`. |
| Sequence number reasonable | `seq_counter` starts at 1, wraps at 1000. |
| Stable repeated packets | Each packet has identical structure, same payload values. |

<a id="part-6"></a>
## Part 6: Does the Counter Output Match Aurora Format?

### 6.1 Short Answer

**No — but you don't need to worry about it.** The Aurora IP itself does the AuroraLink/64B-66B encoding for you. Your pattern generator just produces clean **AXI Stream** (TDATA + TVALID + TREADY + TLAST). Aurora wraps that in its own framing.

### 6.2 What Layers Are There?

```
Layer 4 — Application data         Your 37 × 32-bit words (what we generate)
            ↓
Layer 3 — AXI Stream framing       TDATA + TVALID + TLAST  (what we output)
            ↓
Layer 2 — AuroraLink framing       Aurora IP adds: SOF marker, K-codes, CRC, idle codes
            ↓
Layer 1 — 8B/10B line coding       Aurora IP maps each 8-bit byte to a 10-bit symbol (DC-balanced, embedded clock recovery)
            ↓
Layer 0 — Serial differential pair Bits go onto the GTH transceiver fiber/loopback
```

You only care about Layer 4 and Layer 3. Aurora IP handles 0, 1, 2.

### 6.3 What Counts as "AuroraLink Format"?

When people say "AuroraLink format", they often mean either:

1. **The wire format** — what bits actually flow on the fiber. This is determined by the Aurora IP's configuration (line rate, lane count, line coding 8B/10B vs 64B/66B). **For this design** the configuration is `aurora_8b10b` v11.1 (PG046), 1 lane × 4 byte lane width, 2 Gbps line rate, 125 MHz refclk — sourced from `top.xsa/design_1.hwh`. Both ends must agree on these settings.
2. **The packet structure** — TLAST framing, packet length. This is determined by your AXI Stream signaling.

If RTDS expects packets of "exactly 37 × 32-bit words with TLAST on the last", then your pattern generator output IS in the right format. RTDS doesn't care that the contents are floats vs counters — it just looks at the framing.

### 6.4 The "Smart" Counter Vs Pure Counter

| Approach | Output | Will the SOFTWARE accept it? | Use case |
|----------|--------|------------------------------|----------|
| **Pure counter** (1, 2, 3, ..., 37) | 37 incrementing ints | NO — float words become invalid (small ints look like denormal floats, may trigger NaN check) | Useful for raw link bring-up only — verify Aurora delivers the same numbers we put in. |
| **Smart counter** (Section 5.4) | Valid floats + run=1 + seq | YES — controller will run, no error flags | Use this for end-to-end testing including the controller. |

**Recommendation:** Build BOTH and switch via a parameter or a multiplexer. Use pure counter first; once link works, switch to smart counter.

<a id="part-7"></a>
## Part 7: TX Pin Connections — Where Does Aurora TX Physically Go?

### 7.1 The KR260 Hardware

The Kria KR260 carrier card has multiple physical interfaces that can drive Aurora:

| Interface | Connector type | Signal pair | Usage |
|-----------|---------------|-------------|-------|
| **SFP+ cage 0** | SFP+ optical/copper module slot | `MGTH_TX0_P/N` and `MGTH_RX0_P/N` | Most common for Aurora — plug a fiber transceiver in |
| **SFP+ cage 1** | SFP+ optical/copper module slot | `MGTH_TX1_P/N` and `MGTH_RX1_P/N` | Second SFP+, can be used for second Aurora link |
| **SLVS-EC connector** | High-speed SLVS-EC for cameras | Different pins | Not Aurora |
| **PMOD connectors** | LVCMOS GPIOs | N/A | NOT for Aurora — these are slow GPIOs |

> **Aurora needs a high-speed serial transceiver (GTH on Zynq UltraScale+).** It cannot run on regular GPIO pins. On KR260, that means SFP+ cages.

### 7.2 The GTH Transceiver

Inside the Zynq UltraScale+ (KR260's chip), there are dedicated **multi-gigabit transceivers** called **GTH**. These are the only physical pins that can do Aurora.

```
      Inside the FPGA
      ┌─────────────────────┐
      │   Aurora 8B/10B IP │
      │                     │
      │   parallel data     │
      │   (32 or 64 bits)   │
      └──────┬──────────────┘
             │
             ▼
      ┌─────────────────────┐
      │   GTH Quad          │   "Quad" = group of 4 transceivers + 1 reference clock PLL
      │   ┌─────────────┐   │
      │   │ Channel 0   │   │  ← Aurora uses 1 channel (lane)
      │   │  (TX SerDes)│   │
      │   │  (RX SerDes)│   │
      │   └─────────────┘   │
      └─────┬───────┬───────┘
            │       │
       MGTH_TX_P  MGTH_TX_N    ← differential pair (2 physical pins)
       MGTH_RX_P  MGTH_RX_N
            │       │
            ▼       ▼
       (SFP+ cage on KR260 carrier card)
            │
            ▼
       Optical fiber (or copper SFP+ DAC cable)
            │
            ▼
       (other end — RTDS or loopback)
```

### 7.3 Pin Assignments (Look Up in the KR260 Schematic)

The exact pin numbers come from:
- KR260 user guide / schematic (Xilinx **UG1092** — KR260 Robotics Starter Kit User Guide. **Not** UG1089 — that is the KV260 user guide; **not** UG1093 — that is the KD240 user guide. Confirmed from the AMD Adaptive Computing Wiki, Kria SOMs & Starter Kits page.)
- KR260 Data Sheet: **DS988** (not DS986, which is the KV260 data sheet)
- Carrier Card Design Guide (board-independent): **UG1091**
- Vivado's **KR260 board file**, which auto-assigns pins when you set the board target

> **Confirmed from `top.xsa/design_1.hwh`:** for this exact design the GT reference clock is on package pins **Y6 (P) / Y5 (N)** (`C_REFCLK_LOC_P/N`), routed to GTH **Quad X0Y1 channel X0Y4 clk0** (`C_REFCLK_SOURCE = "X0Y4 clk0"`). The TX/RX serial pin assignments are wired by the board flow when board target is `xilinx.com:kr260_som:1.0`. See `docs/milestone2/M2-A_auroralink_findings.md` §2.

When you create a Vivado project and choose **KR260** as the board, then add the **Aurora 8B/10B** IP, Vivado's IP integrator will:

1. Show a "Board" tab where you can map the Aurora IP's GTH interface to a board interface (e.g., "SFP+ Connector 0").
2. Auto-generate the XDC pin constraints so you don't have to type pin numbers manually.

**If you do NOT use the board flow**, you must write a constraint file (`.xdc`) like:
```tcl
# Example XDC for KR260 SFP+0
set_property PACKAGE_PIN <pin_letter_number> [get_ports {gt_txp[0]}]
set_property PACKAGE_PIN <pin_letter_number> [get_ports {gt_txn[0]}]
set_property PACKAGE_PIN <pin_letter_number> [get_ports {gt_rxp[0]}]
set_property PACKAGE_PIN <pin_letter_number> [get_ports {gt_rxn[0]}]
# Reference clock pin
set_property PACKAGE_PIN <pin_letter_number> [get_ports {gt_refclk_p}]
set_property PACKAGE_PIN <pin_letter_number> [get_ports {gt_refclk_n}]
```

Use the KR260 schematic to fill in the pin letters/numbers. The board-file flow is strongly recommended — it handles this for you.

### 7.4 Reference Clock

Aurora cannot self-clock — it needs an external **GT reference clock**. For **this** design (`aurora_8b10b` v11.1, 2 Gbps line rate, sourced from `design_1.hwh`), the refclk is **125 MHz** and is routed to pins **Y6 / Y5** on the KR260 SOM (differential pair `GT_125_REFCLK_clk_p` / `GT_125_REFCLK_clk_n`).

The Y6/Y5 → SOM240_2 C3/C4 mapping is independently verified against two sources: `design_1.hwh` (`C_REFCLK_LOC_P=Y6`, `C_REFCLK_LOC_N=Y5`) and XTP685 `Kria_K26_SOM_Rev1.xdc` lines 284–285 (`Y6 = MGTREFCLK0P_224 = som240_2_c3`; `Y5 = MGTREFCLK0N_224 = som240_2_c4`).

> **⚠ CRITICAL HARDWARE PRECONDITION (carrier-card-dependent).** The frequency on those SOM pins is set by an oscillator on the *carrier card*, not the SOM. Per XTP743 (KR260 Carrier Card Schematic, page 16), the **stock KR260 starter-kit carrier card** drives `som240_2_c3/c4` from a fixed **156.25 MHz LVDS** oscillator (the standard reference for 10G Ethernet over SFP+ — see UG1092 §SFP+). That is **not** the 125 MHz this Aurora IP expects.
>
> Therefore the supplied bitstream can have worked on only one of:
> 1. a **custom CHIL carrier card** with a 125 MHz oscillator on `GTH_REFCLK0_C2M_P/N` (likely — `xsa.xml` targets the SOM-level board file `kr260_som:1.0`, not the carrier-level `kr260_carrier:1.0`), OR
> 2. a stock KR260 with the GTH_REFCLK0 oscillator **physically replaced**.
>
> Before attempting bring-up, confirm which carrier card you are using. See M2-A doc Gap 10.8 for the full analysis. Do **not** assume a stock KR260 will work without verification.

On KR260 specifically:

- The KR260 SOM exposes the GTH refclk pin pair (Y6/Y5) to SOM connector pins `som240_2_c3` / `som240_2_c4`; the *carrier card* decides what frequency arrives there.
- The Aurora IP wizard asks you to select the reference clock source. The KR260 carrier board file (`kr260_carrier:1.0/board.xml`) does **not** define a `GT_125_REFCLK` board interface, so you cannot use `apply_bd_automation -rule board` here — you must create external ports and apply explicit `PACKAGE_PIN Y6/Y5` constraints (which is what `scripts/vivado/aurora_loopback.xdc` does).

> For reference: PG074 Aurora 64B/66B at 5–10 Gbps uses 156.25 MHz refclks; do **not** copy those numbers into this design (this is a separate issue from the stock-KR260 oscillator above — same number, totally different reason).

<a id="part-8"></a>
## Part 8: Verifying the System WITHOUT an RTDS (or fiber, or second board)

> **⚠ READ THIS BEFORE TOUCHING THE BOARD — two non-obvious gotchas you must handle in the rebuild.**
>
> **Gotcha 1: `loopback[2:0]` is hidden in the supplied IP.** The supplied `top.xsa` bitstream has the Aurora IP's `loopback[2:0]` pin **tied internally to `3'b000`** (normal operation). It is NOT brought out to the IP wrapper's top level. Setting an xlconstant or a runtime register to `3'b010` against this IP will do **nothing** — the IP doesn't see the pin. Last time we wasted a day chasing the same "loopback doesn't work" symptom; the root cause was this. PG046 §"Port Descriptions" confirms `loopback[2:0]` is a *standard* port but exposure at top level depends on the IP customisation. To run any loopback at all you MUST re-customise the IP (see §8.5 below).
>
> **Gotcha 2: refclk frequency on stock KR260 — RESOLVED via lab variant.** On a stock KR260 starter kit, the carrier-card oscillator on the SOM240_2 C3/C4 refclk pins is **156.25 MHz, NOT 125 MHz** (per XTP743 page 16). **Resolution:** rebuild the Aurora IP with `C_REFCLK_FREQUENCY=156.25` and `C_LINE_RATE=2.5` (same integer GTH PLL multiplier as production — ×16). The build script `scripts/vivado/build_aurora_loopback.tcl` accepts this as a command-line argument; defaults are already set for stock KR260. Software, FIFO, IRQ, framing, TLAST behaviour are all frequency-independent and identical at 2 Gbps and 2.5 Gbps. See M2-A doc §12.6 and Gap 10.8.

This Part is reorganised around **what you can prove without an RTDS, and in what order**. Skipping levels = chasing ghosts.

### 8.1 What "RTDS simulator" can mean (pick one — they prove different things)

| Stage | What it is | Hardware needed | What it proves | Effort |
|---|---|---|---|---|
| **S1 — Vivado RTL simulation** | Two Aurora IPs back-to-back in the Vivado simulator using the example testbench shipped with PG046 | None (Vivado on a PC) | Aurora IP customisation is internally consistent: framing, clocks, TLAST round-trip, frame check. Catches IP-config mistakes before any board work. | ~1–2 h |
| **S2 — On-board, pattern generator → AXI FIFO → IRQ → PS software, Aurora NOT in the path** | RTL pattern generator writes 37-word frames into the **RX side** of the AXI FIFO directly (or via a near-zero-latency stream interconnect). Aurora is instantiated but bypassed. | One KR260 | The FIFO + IRQ + DTBO + UIO + libEGD + the `axiReceive` software path are correct, **independently of whether Aurora works**. | ~half day |
| **S3 — On-board PMA Near-End Loopback through Aurora** | Same as S2 but the data path is now Pattern Gen → TX FIFO → Aurora TX → PMA loopback → Aurora RX → RX FIFO → IRQ → PS. **Requires the Aurora IP to be re-customised to expose `loopback[2:0]`.** | One KR260 + a rebuilt bitstream (the supplied one will not do) | Aurora IP comes up (`channel_up=1`), refclk is correct, SERDES round-trips data, framing+TLAST survive the link | ~1 day |
| **S4 — External SFP+ fibre loopback** | SFP+ optical module + 1 m LC-LC fibre patch from TX optical to RX optical of the same cage | One KR260 + SFP+ optical module (~£40) + LC-LC fibre jumper (~£10) | Above + actual physical-optical layer | ~half day |
| **S5 — Real RTDS far end** | Real cable from KR260 SFP+ to RTDS Aurora card | Production setup | The complete production path including RTDS protocol-level interop | as long as the RTDS handshake takes |

### 8.2 Recommended sequence: S1 → S2 → S3

- **S1 is non-negotiable** before you build a bitstream. It is the cheapest, fastest, highest-confidence way to catch IP-customisation mistakes (wrong line rate, wrong framing mode, wrong lane width, wrong refclk frequency, …). The PG046 testbench compares the RX stream against what was sent and prints PASS/FAIL.
- **S2 is the highest leverage on-board step.** It validates the entire software/Linux/FIFO stack with **no dependency on Aurora**. If S2 passes and S3 then fails, you know with certainty that the bug is in Aurora or refclk and not in software.
- **S3 only needs to be attempted after S1 and S2 both pass.** If you go straight to S3 and it fails, you cannot tell which of the ten failure modes in §8.3 below caused it.
- **S4 / S5 are optional for the lab demo.** They are required for production sign-off but not for proving the architecture works.

### 8.3 The ten failure modes of PMA Near-End Loopback (and how to detect each)

PMA Near-End Loopback is the **correct** method to test Aurora without external hardware. It is also the method with the most ways to silently fail. Each row below is a real failure mode we (or others) have hit:

| # | Failure mode | Why it fails silently | Detection / fix |
|---|---|---|---|
| **F1** | **`loopback[2:0]` pin not exposed at the IP top level** (= the supplied design) | IP wrapper ties pin to `3'b000` internally. Your xlconstant connection lands on a stub. | In the Aurora IP customisation GUI, *"Show Optional Ports"* (or similar — exact name varies by IP rev) and tick the **`loopback`** port. Rebuild. Confirm with `report_design_analysis -port_list aurora_8b10b_0` that `loopback` appears in the port list. |
| **F2** | ~~Refclk frequency wrong on stock KR260~~ → **RESOLVED via lab variant.** Stock KR260 delivers 156.25 MHz on SOM240_2 C3/C4 (per XTP743 p.16); we rebuild the Aurora IP for 156.25 MHz + 2.5 Gbps line rate (same integer GTH PLL multiplier as production). | Used to be: GTH PLL cannot lock to a 156.25 MHz source with 125 MHz dividers. Now: not a failure mode — the lab variant matches what hardware delivers. | (Resolved.) Use `scripts/vivado/build_aurora_loopback.tcl` defaults (156.25 MHz / 2.5 Gbps), OR explicitly: `vivado -mode batch -source ... -tclargs --refclk_mhz 156.25 --line_rate_gbps 2.5`. For production switch to 2 Gbps / 125 MHz refclk, invoke with `--refclk_mhz 125 --line_rate_gbps 2` once the right carrier is available. |
| **F3** | **PCS vs PMA loopback confusion** — `3'b001` (PCS) skips the SERDES; `3'b010` (PMA) tests the SERDES | Off-by-one in xlconstant `CONST_VAL`: typing `1` (decimal = 0b001 = PCS) when you meant PMA. The link "comes up" but a SERDES bug is hidden. | xlconstant `CONST_VAL = 2`, `CONST_WIDTH = 3`. Cross-check by toggling between 1 and 2 — if 1 links but 2 doesn't, your SERDES or refclk is the problem; if neither links the issue is upstream. |
| **F4** | **Reset ordering wrong** | Aurora requires a strict sequence (PG046 §"Reset and Power Down"): `gt_reset_in` asserted first, `reset_pb` second, then both deasserted in the reverse order with the appropriate delay. Wrong order = the GTH PLL or the Aurora state machine sticks. | Use a single `proc_sys_reset` instance whose `peripheral_aresetn` drives both — let Vivado handle ordering. Or follow PG046 reset-sequencing table exactly. |
| **F5** | **`channel_up` not observable from PS** | You "set up" the link but have no way to read whether it's up. You write software, see no IRQ, conclude software is broken. | **MANDATORY** for any loopback bring-up: add an `axi_gpio` slave at `0xA002_0000` with `channel_up`, `tx_lock`, `hard_err`, `soft_err`, `frame_err` wired to its GPIO inputs. Then `sudo devmem 0xA002_0000 32` from Linux. Bit 0 = channel_up. If bit 0 = 0 forever, the link is dead — NOT a software problem. |
| **F6** | **TX-side FIFO clock-domain crossing wrong** | pl_clk0 = 100 MHz, Aurora `user_clk_out` = 50 MHz. If the TX AXI-Stream FIFO is set to "Common Clock", it silently mis-samples and the RX path receives garbage. | In Vivado the AXIS Data FIFO IP must be set to **"Clock Type: Independent Clocks"** (asynchronous mode). `s_axis_aclk` = pl_clk0; `m_axis_aclk` = `aurora_8b10b_0/user_clk_out`. |
| **F7** | **Framing-mode mismatch** between the two link partners | Real fiber to RTDS: both ends must agree on Framing vs Streaming. In *PMA loopback* both ends ARE the same IP, so this is moot. Listed here for production. | When fiber-connected to RTDS: confirm the RTDS-side is configured for "Framing" mode with TLAST per packet. |
| **F8** | **TLAST never asserted by the pattern generator** | Aurora in Framing mode uses TLAST as the frame boundary. No TLAST = open-ended frame = RX FIFO accumulates words but never sets the RC interrupt. | In ILA: probe the pattern generator's `m_axis_tlast` and confirm it pulses high exactly on the 37th beat of each frame. In software: read `RDFO` — if it grows past 37 without an IRQ firing, TLAST is missing. |
| **F9** | **IRQ width / DTBO mismatch** (Gap 10.7(d)) | If `C_NUM_F2P_0_INTR_INPUTS≠1`, the kernel-visible IRQ number shifts; your DTBO points to the wrong number; UIO doesn't wake. | `cat /proc/interrupts \| grep uio` after the DTBO is loaded. The hex IRQ number there must match what your DTBO claims. |
| **F10** | **UIO interrupt not re-armed after each delivery** | Linux UIO masks the IRQ on each delivery. You get exactly one IRQ then silence. | After each `poll()` returns and the handler runs, **before the next `poll()`**, write a 4-byte 1 to the uio fd: `write(uio_fd, &one_u32, 4)`. (See M2-A doc §11.4 for the exact code.) |

If S3 (PMA loopback) doesn't work for you, walk this list top-to-bottom, run the detection step for each, and you will hit the cause. Do NOT randomly toggle things.

### 8.4 The three loopback options compared (kept for reference)

| Mode | What it does | Hardware needed | Tests what? |
|------|-------------|-----------------|-------------|
| **A. Internal PMA Near-End Loopback** (S3) | TX serial output is wired back to RX serial input INSIDE the GTH transceiver. The "track" is internal to the chip. | None external; but Aurora IP MUST be re-customised to expose `loopback[2:0]` | The digital chain (Aurora encoder, FIFO, software) **plus the SERDES**. Skips the SFP+ module and the fibre. |
| **B. External fibre loopback** (S4) | A short fibre cable from SFP+ TX optical → SFP+ RX optical on the same cage | 1 SFP+ optical module + 1 LC-LC fibre jumper | All of A plus the SFP+ module and the optical layer. |
| **C. External link to RTDS** (S5) | Real cable from KR260 SFP+ to RTDS Aurora card | RTDS hardware | The complete production path including RTDS protocol-level interop. |

### 8.4.1 Train Analogy (kept for intuition)

- **A. Internal PMA Loopback** = the train pulls out of the depot, but instead of going on real track, the engineers wire the depot's exit door directly back to its entrance. The train circles inside the depot.
  - Pros: No tracks needed. Cheap test.
  - Cons: Doesn't prove the rails work.

- **B. External fiber loopback** = the train pulls out, runs on a short curved track that immediately turns back into the same station.
  - Pros: Tests the actual rail and points (physical fiber + SFP+).
  - Cons: Needs hardware (SFP module + fiber).

- **C. External to RTDS** = real journey to the next city.
  - Pros: Production reality.
  - Cons: Needs RTDS available.

### 8.5 How to *actually* enable PMA Near-End Loopback (step by step)

The supplied IP's loopback pin is hidden (F1). To enable any loopback you MUST rebuild the bitstream from a re-customised Aurora IP. Here is exactly how:

#### Step 1 — Expose the `loopback[2:0]` port (one-time IP re-customisation)

1. Open the Vivado project. Block Design view.
2. Double-click `aurora_8b10b_0` to open the IP customisation GUI.
3. Go to the **"Show optional ports"** section (in newer Aurora IP revs this is on the *Core Options* or *Debug and Control* tab; in some revs it is a checkbox labelled *"Make loopback port available"*).
4. Tick the option that exposes **`loopback`** (sometimes labelled "Add loopback control port to the wrapper").
5. Click OK. Vivado will regenerate the IP wrapper. Confirm that `loopback[2:0]` now appears in the IP's top-level port list (`get_bd_pins aurora_8b10b_0/loopback`).

#### Step 2 — Drive `loopback` to `3'b010` (PMA Near-End)

In the Block Design, add an `xlconstant` and wire it to `loopback[2:0]`:

```tcl
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 loopback_const
set_property -dict [list \
    CONFIG.CONST_VAL  "2"  \
    CONFIG.CONST_WIDTH "3" \
] [get_bd_cells loopback_const]
connect_bd_net [get_bd_pins loopback_const/dout] \
               [get_bd_pins aurora_8b10b_0/loopback]
```

**`CONST_VAL = 2` is critical** — that is `3'b010` (PMA Near-End). `1` is PCS Near-End and skips the SERDES; `4` is Far-End and is useless for unilateral testing. The build script `scripts/vivado/build_aurora_loopback.tcl` already does this; **but** that script assumes you have already done Step 1 first. Without Step 1 the `connect_bd_net` lands on a port that doesn't exist and the script will fail with a clear error (which is better than the silent failure of leaving it tied to 0).

#### Step 3 — Add an `axi_gpio` to read `channel_up` (mandatory observability, fixes F5)

```tcl
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_status
set_property -dict [list \
    CONFIG.C_GPIO_WIDTH "5"     \
    CONFIG.C_ALL_INPUTS "1"     \
] [get_bd_cells axi_gpio_status]
# Concat: bit 0 = channel_up, bit 1 = tx_lock, bit 2 = hard_err,
#         bit 3 = soft_err,   bit 4 = frame_err
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 status_concat
set_property CONFIG.NUM_PORTS "5" [get_bd_cells status_concat]
connect_bd_net [get_bd_pins aurora_8b10b_0/channel_up]   [get_bd_pins status_concat/In0]
connect_bd_net [get_bd_pins aurora_8b10b_0/tx_lock]      [get_bd_pins status_concat/In1]
connect_bd_net [get_bd_pins aurora_8b10b_0/hard_err]     [get_bd_pins status_concat/In2]
connect_bd_net [get_bd_pins aurora_8b10b_0/soft_err]     [get_bd_pins status_concat/In3]
connect_bd_net [get_bd_pins aurora_8b10b_0/frame_err]    [get_bd_pins status_concat/In4]
connect_bd_net [get_bd_pins status_concat/dout]          [get_bd_pins axi_gpio_status/gpio_io_i]
# Map to AXI-Lite at 0xA002_0000 via the existing HPM0 interconnect
apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config { Master "/zynq_ultra_ps_e_0/M_AXI_HPM0_FPD" Clk "Auto" } \
    [get_bd_intf_pins axi_gpio_status/S_AXI]
```

After load, from Linux:

```bash
sudo devmem 0xA002_0000 32
# Expect bit 0 = 1 (channel_up). If 0 forever -> link dead -> NOT a software problem.
# Inspect bits 1-4 to localise: tx_lock=0 means refclk; *_err non-zero means line corruption.
```

#### Step 4 — Drive the link with a pattern generator (and verify TLAST, fixes F8)

The pattern generator in §5.4 of this doc emits 37-word frames with TLAST on word 37 and a configurable inter-packet gap. Wire its `m_axis` to the **TX-side** AXI FIFO's `S_AXIS`. Add an ILA probe on `m_axis_tlast` and visually confirm in Hardware Manager that it pulses on every 37th beat.

#### Step 5 — Read back from the RX FIFO via UIO (fixes F9, F10)

§11.4 of `docs/milestone2/M2-A_auroralink_findings.md` has the exact UIO + mmap + poll + re-arm code, with PG080-verified register offsets.

#### Step 6 — Verify TLAST survives the loopback round-trip

Aurora 8B/10B in Framing mode preserves TLAST across the link (PG046 §"Framing Interface"). A 37-word frame sent on `s_axi_tx` comes out as a 37-word frame on `m_axi_rx` with TLAST on the 37th beat. This is the whole point of Framing mode and the reason we picked it.

Verification: in software, after RC IRQ fires, read `RDFO` (RX occupancy). Expect exactly **37**. Then read `RLR` (RX length register). Expect exactly **148** bytes (= 37 × 4). Then read `RDFD` 37 times and verify the pattern matches what the pattern generator sent.

### 8.6 Recommendation for our bring-up — the right phase ladder

This replaces the older 4-phase plan with one that matches the S1–S5 strategy in §8.1:

```
Phase 0 (S1):  PG046 example testbench in Vivado simulator.
               Goal: prove the Aurora IP customisation is internally consistent.
               No board needed. PASS criterion: testbench self-checking prints PASS.

Phase 1 (S2):  Pattern generator -> AXI FIFO -> IRQ -> PS software, Aurora bypassed.
               Goal: prove the FIFO + IRQ + UIO + software chain works on real silicon,
               WITHOUT depending on Aurora.
               PASS criterion: A53 software prints "received 37 words, TLAST on word 37".

Phase 2 (S3):  Phase 1 routed through Aurora in PMA Near-End Loopback.
               Goal: prove the Aurora IP comes up (channel_up=1), refclk works, and
               framing survives the loopback round-trip.
               PASS criterion: devmem 0xA002_0000 -> bit 0 = 1 AND Phase 1 still passes.

Phase 3 (S4, optional):  Phase 2 with external SFP+ + fibre loopback.
               Goal: add the physical/optical layer.

Phase 4 (S5):  Disable loopback; connect to real RTDS.
               Production test.
```

Phase 0 is mandatory before Phase 1, and Phase 1 before Phase 2. **Skipping levels is the single best way to waste a day staring at a dead link.**

### 8.7 The "closed loop" question — does the controller's 23-word TX come back as RX?

Important subtlety in PMA loopback that catches people: in **production**, RTDS sends 37 words → Kria controller computes → Kria sends 23 words back → RTDS uses those 23 to simulate a new step → RTDS sends the next 37. Two distinct frame sizes in the two directions.

In **PMA Near-End Loopback** the GTH does NOT distinguish between "our TX from the pattern generator" and "our TX from the A53 controller output" — both go through the same TX serial line and both loop back as RX. So when the controller writes its 23-word TX, **that 23-word frame ALSO arrives at the RX FIFO** and the software will print *"Warning: receive Length mismatch. Received 23 words. Expected 37."*

**Three ways to handle this in the lab:**

| Option | What you do | Pros | Cons |
|---|---|---|---|
| **OptA — Decoupled (RECOMMENDED for our demo)** | Test RX and TX independently. Phase A: pattern gen → loopback → RX → A53 (verify RX path). Phase B: with the pattern gen *disabled* and `loopback=3'b000`, write A53 TX → ILA captures the 23 words (verify TX path). | Cleanest. Reuses the same bitstream with one runtime toggle. | Doesn't *demonstrate* the closed loop. |
| **OptB — Two Aurora instances** | Instantiate a second Aurora IP. Aurora0 = pattern gen → loopback → A53 RX. Aurora1 = A53 TX → ILA only, no loopback. | Both directions run simultaneously and observable. | Twice the IP area; more clocks to constrain. |
| **OptC — FPGA-side RTDS emulator** | Replace pattern gen with a state machine that consumes the A53's 23-word TX and produces the next 37-word RX as a function of it. | Demonstrates a true closed loop. | Building a tiny RTDS in the FPGA. Major engineering project. Only do this if RTDS will be unavailable for *months*. |

**Recommendation for our timeline:** OptA. Run Phase A and Phase B separately, document both passes, and tell the reviewer truthfully: *"Each direction is verified independently. Full closed-loop interop is gated on RTDS availability (Gap 10.1) and is out of scope of the on-board verification."* That is a defensible position; OptC pretends to do more than the demo actually proves.

<a id="part-9"></a>
## Part 9: Clock and Reset — Detailed Explanation

This is the part where most people get stuck. Read carefully.

### 9.1 Why Are There So Many Clocks?

In the FPGA, different blocks need different clock speeds for different reasons:

| Clock | Frequency for THIS design | Driven by | Used by |
|-------|---------------------------|-----------|---------|
| **PS clock (PL clock 0)** | 100 MHz | Zynq MPSoC (PL_CLK0 from PS) | Pattern generator, FIFOs, ILA, AXI-Lite slaves |
| **GT reference clock** | **125 MHz** (per `top.xsa/design_1.hwh` — `C_REFCLK_FREQUENCY=125`, KR260 pins **Y6/Y5**) | KR260 board oscillator | Aurora's GTH PLL |
| **Aurora user clock** (`user_clk_out`) | **50 MHz** (per `design_1.hwh` — derived from 2 Gbps × 8/10 ÷ 32-bit lane) | Aurora IP itself (output) | Aurora's user-side AXI Stream port, drives the RX FIFO `axis_aclk` |
| **Aurora init clock** | **50 MHz** (per `design_1.hwh` — `C_INIT_CLK=50`) | PL clock generator | Aurora IP's internal init state machine |
| **AXI-Lite clock** | same as PS clock | PS_CLK0 | Aurora's `s_axi_lite` and FIFO control register |

> **Why these numbers and not 5/10 Gbps "standard" Aurora numbers?** This design is `aurora_8b10b` v11.1 (PG046) at 2 Gbps line rate — confirmed from the hardware handoff `design_1.hwh` extracted from `top.xsa`. 64B/66B Aurora (PG074) at 5–10 Gbps uses 156.25 MHz refclks and ~161 MHz user clocks; that is **not** our design. Verify in your Vivado project by opening the Aurora IP customisation dialog after build and reading the `Line rate`, `Refclk`, and `User clock frequency` fields.

### 9.2 Train Analogy for Clocks

Imagine multiple trains running on different time schedules:
- Local trains (PL fabric blocks) run at 100 mph (PS pl_clk0).
- Express trains (Aurora user side) run at 50 mph (Aurora's `user_clk_out` = 50 MHz, derived from line rate).
- The GTH "engine room" runs on its own atomic GT reference clock at 125 MHz.

To pass a wagon between trains of different speeds, you need a **transfer station** (an asynchronous FIFO) where the wagon waits briefly. This is why we put an async FIFO between pattern generator (100 MHz pl_clk0) and Aurora (50 MHz `user_clk_out`).

### 9.3 The Full Clock Map

```
┌──────────────────────────────────────────────────────────────────────┐
│                          KR260 PS                                    │
│                                                                      │
│   ┌──────────┐                                                       │
│   │PS_CLK    │   pl_clk0 = 100 MHz (configurable in Zynq IP)         │
│   │  source  │ ────────────────────────────────────────►             │
│   └──────────┘                                                       │
└──────────┼───────────────────────────────────────────────────────────┘
           │
           │  (this is `aclk` for everything in PL except Aurora's user-side)
           ▼
┌──────────────────────────────────────────────────────────────────────┐
│                          KR260 PL (FPGA)                             │
│                                                                      │
│  ┌────────────────────┐                                              │
│  │ proc_sys_reset     │ ◄── pl_resetn from PS                        │
│  │ (synchronous       │ ◄── pl_clk0  from PS                         │
│  │  reset generator)  │                                              │
│  │                    │ ── peripheral_aresetn ─────────►   to all   │
│  │                    │ ── interconnect_aresetn ───────►   blocks   │
│  └────────────────────┘                                              │
│                                                                      │
│  ┌────────────────┐  ┌─────────────┐    ┌──────────────────┐         │
│  │Pattern Gen     │  │ TX FIFO     │    │ Aurora 8B/10B   │         │
│  │aclk = pl_clk0  │  │ aclk = pl_  │    │                  │         │
│  │aresetn=periph_ │  │ clk0        │    │  init_clk = pl_  │         │
│  │aresetn         │  │ aresetn=    │    │      clk0 (or    │         │
│  └───────┬────────┘  │ peripheral_ │    │      its own)    │         │
│          │           │ aresetn     │    │                  │         │
│          │ AXIS      │             │    │  user_clk = OUT  │         │
│          ▼           │             │    │      (Aurora IP  │         │
│      s_axis ────►    │ m_axis ────►│ s_axi_tx_aclk = AURORA_USER_CLK │
│                      └─────────────┘    │                  │         │
│                                         │  gt_refclk_p/n =                       │
│                                         │  external 125 MHz │         │
│                                         │  (KR260 Y6/Y5)   │         │
│                                         └──────────────────┘         │
│                                                                      │
│  ┌────────────────────┐                                              │
│  │ AXI4-Stream FIFO   │                                              │
│  │ (RX, PG080)        │                                              │
│  │                    │                                              │
│  │ axis_aclk =        │                                              │
│  │   aurora_user_clk  │ ◄── note: same clock as Aurora's RX side     │
│  │                    │                                              │
│  │ axi_lite_aclk =    │                                              │
│  │   pl_clk0          │ ◄── different clock for register reads       │
│  └────────────────────┘                                              │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

### 9.4 Step-by-Step Connections

Here is precisely what to connect in Vivado Block Design:

#### Pattern Generator
- `aclk` → `Zynq.pl_clk0`
- `aresetn` → `proc_sys_reset.peripheral_aresetn`

#### TX AXIS Data FIFO (small, between PatternGen and Aurora)
- `s_axis_aclk` → `pl_clk0`
- `s_axis_aresetn` → `peripheral_aresetn`
- `m_axis_aclk` → **Aurora's `user_clk_out`** (this FIFO crosses clock domains!)
- `m_axis_aresetn` → reset synchronized to user_clk (Aurora provides this, OR use a second `proc_sys_reset` running on `user_clk`)

> **This is a clock-crossing FIFO!** Pattern gen runs at 100 MHz (pl_clk0), Aurora user side runs at 50 MHz (`user_clk_out`, derived from 2 Gbps line rate ÷ 32-bit lane × 8/10). The Data FIFO must be configured as **asynchronous** (independent input/output clocks). Set this in the FIFO IP configuration: "Clock Type" = "Independent Clocks".

#### Aurora 8B/10B
- `init_clk` → `pl_clk0` (for the IP's internal init state machine)
- `gt_refclk1_p`, `gt_refclk1_n` → external **125 MHz** differential pair on KR260 (board pins **Y6/Y5** per `design_1.hwh`; the KR260 board flow connects this automatically when the Aurora IP is associated with the SFP+ board interface)
- `reset_pb` → `peripheral_aresetn` inverted, OR another reset signal (active high typically — check IP doc)
- `pma_init` → either tied low or driven by a "PMA reset" pulse (use the wizard default)
- `s_axi_tx_aclk` → comes out of Aurora as `user_clk_out` (this is its own user clock)
- `s_axi_tx_aresetn` → use Aurora's own reset output, OR a synced reset

#### RX AXI4-Stream FIFO (PG080)
- `s_axis_aclk` → Aurora's `user_clk_out` (RX side)
- `s_axis_aresetn` → reset synced to user_clk
- `s_axi_aclk` (the AXI-Lite control side) → `pl_clk0`
- `s_axi_aresetn` → `peripheral_aresetn`

#### proc_sys_reset (the reset generator)
- `slowest_sync_clk` → `pl_clk0` (or whichever is the slowest you care about — usually pl_clk0)
- `ext_reset_in` → Zynq's `pl_resetn` (or `pl_resetn0`)
- `peripheral_aresetn` → output, fans out to most blocks
- `interconnect_aresetn` → output, used by AXI interconnects

### 9.5 Two `proc_sys_reset` Instances?

If your design has multiple clock domains (and ours does — pl_clk0 + Aurora user_clk), you need **TWO** `proc_sys_reset` IP instances:

| Instance | `slowest_sync_clk` | Purpose |
|----------|-------------------|---------|
| `rst_pl_clk0` | `pl_clk0` | Resets blocks running on the 100 MHz clock |
| `rst_aurora_user` | Aurora's `user_clk_out` (50 MHz in this design) | Resets blocks running on Aurora's user clock |

Both can use Zynq's `pl_resetn0` as their `ext_reset_in`. Each generates a synchronized active-low reset for its own clock domain.

### 9.6 What Vivado's "Run Connection Automation" Does

When you click **Run Connection Automation** after dropping IP in BD:

- It usually figures out clocks/resets for AXI-Lite paths automatically.
- It may NOT correctly handle the streaming side's clock crossing — verify manually.
- For Aurora, it may not even attempt the user_clk side — you typically wire that yourself.

> **Tip:** Don't blindly accept Connection Automation. Open the BD validation report and ensure every block's `*_aclk` and `*_aresetn` is connected to a sensible source.

### 9.7 Reset Order (Important for Aurora)

Aurora's bring-up sequence is sensitive:

1. Power applied.
2. PS_CLK starts.
3. `pl_resetn0` deasserts (PS releases reset).
4. `proc_sys_reset` produces clean synchronous resets.
5. Aurora's `init_clk` starts → IP performs internal reset sequence.
6. Aurora performs **PMA reset** (`pma_init` pulse).
7. Aurora attempts handshake with the other end (or with itself in loopback).
8. After successful handshake, Aurora asserts `channel_up = 1` and starts producing `user_clk_out`.
9. AXI Stream interface is now usable.

> If `channel_up` is stuck at 0, your link isn't working. Check (a) ref clock present, (b) loopback enabled if no fiber, (c) reset sequence complete.

<a id="part-10"></a>
## Part 10: Why Do We Need ILA? Is Debug Required?

### 10.1 Short Answer

**ILA is strongly recommended but not strictly mandatory.** During first bring-up it is essentially mandatory because without it you have no way to see what's happening on the AXI Stream wires inside the FPGA.

### 10.2 What is ILA?

ILA = **Integrated Logic Analyzer**. It is a Xilinx debug IP that:

- Lives inside the FPGA fabric.
- Samples specified signals on every clock edge into an on-chip BRAM buffer.
- When triggered (e.g., on `tlast=1`), captures a window of samples (e.g., 1024 cycles).
- Lets you view those samples as a waveform in Vivado Hardware Manager — like a real oscilloscope, but for digital signals inside the chip.

### 10.3 Train Analogy

ILA = security cameras on the platform. Without cameras:
- Train arrives → conductor says "passenger missing!" → you have no idea why.
- With cameras: you replay the footage → "ah, passenger fell off at wagon 12 — wagon connection broken."

### 10.4 What to Probe With ILA — Recommended Signal List

Probe these signals (put them all on one ILA, configurable trigger):

**Pattern Generator output (TX side):**
- `m_axis_tdata[31:0]`
- `m_axis_tvalid`
- `m_axis_tready`
- `m_axis_tlast`

**Aurora link status:**
- `channel_up` — high when link is up (most important signal!)
- `lane_up` — per-lane handshake (for multi-lane Aurora; for our single-lane it's same as channel_up)
- `gt_pll_lock` — high when GT PLL is locked
- `crc_pass_fail_n` — high when CRC OK on RX

**Aurora RX output (after loopback):**
- `m_axi_rx_tdata[31:0]`
- `m_axi_rx_tvalid`
- `m_axi_rx_tlast`

**RX FIFO:**
- `axis_str_rxd_tvalid` (the data going into FIFO from Aurora)
- `interrupt` (output to GIC)

**FIFO control (optional):**
- `prog_full`, `prog_empty` (almost-full/empty flags) — tells you backpressure status

### 10.5 Setting Up ILA

1. Add **ILA (Integrated Logic Analyzer)** IP from the IP catalog into your Block Design.
2. Configure the ILA IP:
   - **Number of probes:** ~12–15 for the list above.
   - **Sample data depth:** 1024 or 4096 (more = more BRAM used).
   - **Probe widths:** match the signals (e.g., probe0 = 32 bits for tdata, probe1 = 1 bit for tvalid, etc.).
3. Connect the probes by drawing wires from the source signals to the ILA's probe inputs.
4. Set the ILA's `clk` to whichever clock the signals run on (most often `pl_clk0`, but for Aurora-side signals it must be Aurora's `user_clk`). You may need TWO ILAs if you have signals on different clocks.

> **Tip:** During synthesis, Vivado will sometimes optimize signals away. To prevent this, mark them with `(* mark_debug = "true" *)` in your RTL OR use the "Set Up Debug" wizard.

### 10.6 Triggering — How to Catch the Right Moment

Without a trigger, ILA captures continuously and overwrites old samples. You want to capture **around an interesting event**.

Useful triggers:
- `m_axis_tlast == 1` → captures every packet boundary
- `channel_up == 0 → 1` (rising edge) → catches the moment the link comes up
- A specific tdata value (e.g., the magic number `0xCAFE0001` on word 1)
- An ERROR pulse (e.g., `crc_pass_fail_n == 0`)

### 10.7 When You Can SKIP ILA

After everything works and you trust the link, ILA mostly costs FPGA area and BRAM. You can:

- Disable the ILA in your final bitstream (or remove the IP entirely).
- Keep just one minimal ILA on `channel_up` for occasional diagnostics.

For development → keep ILA. For production → optional.

### 10.8 ILA vs Software Logging

| Method | Pros | Cons |
|--------|------|------|
| ILA | See actual hardware signals at clock-cycle precision. No software needed. | Limited to small windows of capture. |
| `xil_printf` from C | Easy to add. Good for tracking software logic. | Cannot see raw FPGA wires. Slow (~1 ms per print). |
| Both | Use ILA for FPGA-side timing; printf for software state. | Best practice. |

> **For our PEBB system:** ILA is essential for verifying the FIFO interrupt fires when expected, that 37 words actually got into the FIFO, and that the loopback round-trip is intact. Software printf (in `xil_printf`) confirms the controller is interpreting them correctly.

<a id="part-11"></a>
## Part 11: Buffer/FIFO Handling — Race Conditions and Backpressure

This is the part where production systems usually break in the field. Read carefully.

### 11.1 The Five Possible States of the RX FIFO

At any given moment the RX FIFO can be in one of these states:

| State | Description | What happens to incoming data? | What software sees |
|-------|-------------|-------------------------------|--------------------|
| Empty | No words queued | New word arrives, gets stored. | Occupancy=0, no interrupt. |
| Partial | Some words of current packet are in, but TLAST not yet seen | New word stored, occupancy increases. | No interrupt yet (waiting for TLAST). |
| One complete packet | TLAST received once. CPU not yet drained it. | New word stored if it's start of next packet. | Interrupt asserted. |
| Multiple complete packets | TLAST received multiple times. CPU is slow. | New word stored if there's still capacity. | Interrupt remains asserted. |
| Full / Overflowing | FIFO is full. Backpressure (`tready=0`) propagates upstream. | New incoming words are blocked at Aurora. | Aurora may drop link (if it can't push back). |

### 11.2 Backpressure — How AXI Stream Prevents Overflow

The AXI Stream protocol has a built-in flow-control mechanism: **TREADY**.

- If RX FIFO is full → it deasserts `s_axis_tready` to Aurora.
- Aurora sees `tready=0` → it stops outputting words.
- Aurora's internal buffers may absorb a few words, then it must somehow tell the upstream sender "stop".
- For a real link to RTDS, RTDS cannot stop (it's hard real-time). Aurora's CRC/sequence numbers allow it to detect if data was dropped, but recovery is up to the user.
- For our loopback test, the pattern generator sees `tready=0` on its `m_axis_tready` and pauses — perfectly safe.

### 11.3 Train Analogy for Backpressure

- The arrival platform has 5 wagon spots.
- Wagons keep arriving. Spots fill up.
- The arrival platform staff raises a "STOP" flag (`tready=0`) to the railway controller.
- Trains coming in stop at the previous station.
- When the station master finally inspects and clears wagons, the "STOP" flag drops, trains resume.
- For RTDS (which can't stop) — wagons would simply be dropped into a ditch (link errors).

### 11.4 What If a Packet Is Being Read When New Data Arrives?

This is your key question — let's trace it carefully.

**Scenario:** Software is in the middle of `axiReceive()`, calling `XLlFifo_RxGetWord()` for, say, the 20th word. Meanwhile, RTDS sends the next 37-word packet.

```
Time → 

t=0    : Packet N arrives fully. Interrupt fires. CPU starts FifoHandler → axiReceive.
t=10us : axiReceive is reading word 5 of packet N.
t=~125us: Packet N+1 starts arriving from RTDS. (Period set by RTDS model; 125 µs / 8 kHz is the *assumed* target — see RTDS doc gap in §1.1.)
         FIFO accepts these new words into its memory (FIFO is multi-packet capable!).
t=130us: axiReceive still reading words ~6 of packet N. Packet N+1 fully arrived.
         Interrupt remains asserted (or re-asserts) for packet N+1's RC.
t=180us: axiReceive completes packet N. Returns from FifoHandler.
         FifoHandler loop sees `Pending` is still non-zero — calls FifoRecvHandler again.
         OR: GIC fires the interrupt again immediately.
t=181us: axiReceive starts on packet N+1 (now the "current" packet in the FIFO).
```

**This works because:**

1. The Xilinx **AXI4-Stream FIFO IP (PG080)** stores **multiple complete packets**. Each packet's length is preserved separately — `XLlFifo_iRxGetLen()` returns the length of the **current** packet only.
2. After draining all 37 words of packet N, the next call to `XLlFifo_iRxGetLen()` returns 37 again — but for packet N+1.
3. The RC interrupt remains asserted as long as there's at least one complete packet in the FIFO.

### 11.5 Important: Packet Boundaries Are Preserved

Even if multiple packets are queued in the FIFO, they don't get mixed:

```
FIFO contents (multi-packet):

  ┌─[Packet N]──────────────┐ ┌─[Packet N+1]────────────┐ ┌─[Packet N+2]
  │w1 w2 w3 ... w36 w37 [end]│ │w1 w2 w3 ... w36 w37 [end]│ │w1 w2 ...
  └──────────────────────────┘ └──────────────────────────┘ └────────

XLlFifo_iRxGetLen() returns 37 (length of packet N).
XLlFifo_RxGetWord() x 37 → drains packet N.
XLlFifo_iRxGetLen() returns 37 again (length of packet N+1).
XLlFifo_RxGetWord() x 37 → drains packet N+1.
```

PG080 FIFO has internal logic to keep track of where each packet boundary is. As a software developer, you just keep calling `iRxGetLen()` and `RxGetWord()` until occupancy is 0.

### 11.6 What If the FIFO Fills Up Before Software Drains?

If the controller is slow enough that the RX FIFO fills with multiple packets before software catches up:

1. New incoming words start to backpressure (`tready=0`).
2. Aurora's internal small buffer fills.
3. If we're in **loopback mode**, the pattern generator's `m_axis_tready` becomes 0 → it pauses → no data lost (just delayed).
4. If we're connected to **real RTDS**, RTDS keeps sending. Aurora cannot push back to RTDS → words are dropped or CRC errors fire.
5. Software detects this via `commGlitchCtr++` (length mismatches) and `Rx_Done` overrun checks (sets `intOverrunFlag`).

### 11.7 The `intOverrunFlag` Check

In `main.c` `FifoHandler`:
```c
if (Rx_Done) {
    intOverrunFlag = 1;  // previous packet not yet processed!
}
FifoRecvHandler(InstancePtr);
```

This sets a flag if a new RC interrupt arrives BEFORE `Rx_Done` was cleared by the previous `runControlCycle()` finishing. It's a software-side timing-overrun warning.

### 11.8 Race Conditions to Worry About

| Race | What can go wrong | Mitigation |
|------|-------------------|------------|
| Software reads while hardware writes | `iRxGetLen()` returns mid-packet length | PG080 hardware atomicity — length register reflects only complete packets. Safe. |
| Interrupt fires while CPU is in handler | Recursive interrupt | GIC masks the interrupt while the handler runs (priority/preemption disabled) — re-enters when handler exits. Safe. |
| Multiple CPUs reading same FIFO | Concurrency hazard | Our system uses only CPU0. Safe by configuration. |
| Software ACK before drain | Premature interrupt clear | Code clears RC flag at start of handler (line 432 of main.c), then drains. If new packet arrives during drain, RC re-asserts → handled in `while(Pending)` loop. Safe. |

### 11.9 Backpressure Through the Whole Chain

In our loopback test, the chain is:
```
PatternGen ── m_axis ──► TX FIFO ── m_axis ──► Aurora TX ──► loopback ──► Aurora RX ──► RX FIFO ──► CPU
```

Backpressure propagates BACKWARDS:
- If RX FIFO is full → it deasserts its `s_axis_tready` to Aurora RX.
- Aurora RX's tiny internal buffer fills (a few words at most).
- Aurora cannot easily push back through the link — for loopback, it pauses Aurora TX (the same IP).
- Aurora TX deasserts its `s_axi_tx_tready` → TX FIFO can't drain.
- TX FIFO fills, deasserts `s_axis_tready` → pattern generator pauses.

**End result:** in loopback, the pattern generator naturally pauses if the CPU is too slow. No data lost. Self-throttling.

### 11.10 Train Analogy for the Whole Chain

```
[Train Driver]──pauses──► [Loading Platform fills]──pauses──► [Engine TX]──pauses──► 
   ──► [Track]──► [Engine RX]──pauses──► [Arrival Platform fills]──tready=0──► 
   [Station Master overwhelmed]
```

When the station master is busy, the "STOP" flag propagates all the way back to the train driver, who pauses at the depot. When the master clears wagons, "STOP" lifts, traffic resumes.

<a id="part-12"></a>
## Part 12: Complete Closed-Loop Cycle — Counter → Controller → Switches → Back

This is where we put everything together. Trace the journey of one piece of data through the WHOLE system.

### 12.1 The Cycle (One Iteration)

```
1. Pattern Gen produces 37 words → packet N
2. Packet N → TX FIFO → Aurora TX → loopback → Aurora RX → RX FIFO
3. RX FIFO asserts RC interrupt
4. A53 CPU jumps to FifoHandler → FifoRecvHandler → axiReceive
5. axiReceive reads 37 words, applies type/scale conversion, stores into:
     PEBB_Control_U.feedback[0..10]   (currents, voltages)
     P_CMD, Q_CMD, V_CMD               (power commands)
     run, RXI_chil, seq                (control flags)
     etc.
6. runControlCycle is called:
     - Checks run==1, ctrlOn==1, ReceiveLength==37
     - Calls PEBB_Control_step():
         reads PEBB_Control_U inputs
         computes:
           PEBB_Control_Y.switch_1[0..2]    (3-phase duty cycles)
           PEBB_Control_Y.enable
           PEBB_Control_Y.lowside_contact
           ...etc.
     - Calls TxSend() → axiSend()
7. axiSend packs 23 words, writes to TX FIFO via XLlFifo_TxPutWord
8. axiSend calls XLlFifo_iTxSetLen(23 * 4 = 92 bytes) → triggers Aurora TX
9. The 23 words travel: TX FIFO → Aurora TX → fiber/loopback → ?
10. Software returns to while(1), waits for next interrupt
```

### 12.2 What Happens to the 23 TX Words?

This is where loopback gets tricky.

**Case A — Pure PMA loopback (one Aurora link):**

The 23-word TX packet goes through the loopback path and arrives back at the SAME Aurora's RX. The RX FIFO sees a packet with `ReceiveLength = 23`, NOT 37.

- `axiReceive()` runs.
- `if (ReceiveLength != RECEIVE_LENGTH)` → `Warning: receive Length mismatch. Received 23 words. Expected 37.` is printed.
- `commGlitchCtr` increments. After 10 such glitches → `CHIL_error |= 1`.

**This is the failure mode you have to be aware of.** Pure loopback does NOT close the controller loop in the way RTDS would.

**Case B — Two separate Aurora links (Option A from Part 8.7):**

- Aurora #0 (RX path) receives pattern gen's 37-word packets.
- Aurora #1 (TX path) sends the controller's 23-word output to a separate sink (e.g., terminated, or to another ILA).
- No interference. Software sees only the 37-word packets from Aurora #0.

**Case C — RTDS Emulator block (Option B from Part 8.7):**

A custom Verilog block:
- Listens to Aurora's TX (the controller's 23-word output).
- Maintains a simulated power-system state.
- Generates the next 37-word packet whose values reflect that state.
- Pushes the new 37-word packet into the SAME Aurora's TX again to loop back as RX.

Complex but is true closed-loop testing without RTDS.

### 12.3 Recommended Lab Topology for This Project

```
For Phase 1 testing (this document):

  [Pattern Gen 37w] ──► TX FIFO_a ──┐
                                    │ multiplexer (only one source at a time)
  [Software TX 23w] ──► TX FIFO_b ──┘
                                    │
                                    ▼
                              [Aurora TX] ──► PMA loopback ──► [Aurora RX]
                                                                      │
                                                                      ▼
                                          [RX FIFO with packet filter]
                                                                      │
                                                                      ▼
                                                                    [A53]

Filter rule: only forward 37-word packets to A53. Discard 23-word packets
            (or capture them in ILA for verification).
```

But this requires extra logic. The **simplest "good enough" approach** for the lab is:

```
For Phase 1 testing (simple):

  [Pattern Gen 37w] ──► TX FIFO ──► Aurora TX ──► PMA loopback ──► Aurora RX ──► RX FIFO ──► A53

  [Software TX] ──► (nowhere; A53 still calls axiSend, but it just goes to /dev/null)
                    OR: Software TX → second TX FIFO → second Aurora TX → ILA only
                    OR: Disable software's axiSend by setting ctrlOn=0 via UART
```

In `main.c` you can disable the controller call (and thus axiSend) by leaving `ctrlOn = 0` (controlled via UART command). Then `runControlCycle` skips `PEBB_Control_step` and `TxSend`. Pattern gen → axiReceive works fine, no TX traffic competes for the link.

### 12.4 Verifying the Cycle Works

Build a checklist:

| Stage | How to verify | Expected result |
|-------|--------------|-----------------|
| Pattern gen producing data | ILA on `m_axis_*` of pattern gen | See 37 valid words per packet, TLAST on word 37 |
| TX FIFO accepting | ILA on `m_axis_tready` (back to pattern gen) | tready stays high (never blocks) |
| Aurora link up | ILA on `channel_up` | Steady high after ~1 second post-boot |
| Loopback round trip | ILA on `m_axi_rx_*` | Same 37 words arrive back, with TLAST |
| RX FIFO interrupt | Probe `interrupt` signal | Pulses on every packet |
| A53 receives interrupt | UART log `xil_printf` in FifoHandler | Print on each interrupt (add temporarily) |
| axiReceive succeeds | UART log no "Length mismatch" warnings | Quiet log = 37 words received |
| Variables populated | UART log of `PEBB_Control_U.feedback[0]` value | Should be 1500.0 (= 1.5 * 1000 scale) |
| Controller runs | UART log "controllerStarted = 1" | True after ~1 second |
| TX 23 words | ILA on Aurora TX `s_axi_tx_*` | 23 words with TLAST after every controller step |

### 12.5 Where Variables End Up — Tracking 1.5 kA

You configured pattern gen word 2 = `0x3FC00000` (= 1.5 in float).

```
Pattern Gen → 0x3FC00000
    ↓ AXI Stream (TDATA)
TX FIFO → 0x3FC00000
    ↓
Aurora TX (encodes bytes to 8b/10b symbols, serialises at 2 Gbps)
    ↓ loopback
Aurora RX (decodes back to 32 bits)
    ↓
RX FIFO stores 0x3FC00000 in slot
    ↓ interrupt fires
A53 reads via XLlFifo_RxGetWord() → RxWord = 0x3FC00000
    ↓
axiReceive: rxData[1].axiType=float, localType=double, scale=1000
RxValue = *(float*)&RxWord = 1.5f
*((double*)rxData[1].ptr) = 1.5f * 1000 = 1500.0
    ↓
PEBB_Control_U.feedback[0] = 1500.0 (now in RAM as a double)
    ↓
PEBB_Control_step() reads feedback[0]
"current is 1500A. compute new switch duty cycles"
    ↓
PEBB_Control_Y.switch_1[0] = 0.75 (some computed duty)
    ↓
axiSend: txData[0] points to switch_1[0], axiType=float, scale=1
float txWord = (float)0.75 * 1.0 = 0.75 = 0x3F400000
XLlFifo_TxPutWord(0x3F400000)
    ↓
TX FIFO → Aurora TX → out the SFP+ pin (or loopback)
```

The full data lifecycle is:
- IEEE 754 bytes → reinterpreted as float → scaled → stored as double
- Double read by controller → computed → cast back to float → reinterpreted as bytes → sent

### 12.6 Closed-Loop Behavior (for reference, not implemented in this doc's scope)

If you DO build the RTDS emulator (Option B), the cycle becomes:

```
Cycle 0: Emulator generates packet with i_high = 0 A
Cycle 1: A53 controller computes: "no current, ramp up duty to 0.5"
         Sends 23 words back, switch_1[0] = 0.5
Cycle 2: Emulator updates simulated state: "with 0.5 duty, current grows to 0.5 kA"
         Sends new packet with i_high = 0.5 kA
Cycle 3: A53 controller: "current 500A, target 1000A, increase to 0.7"
         ...and so on, until steady state
```

This is where you'd see the controller actually do its job. The smart counter pattern generator does NOT have this state-update logic — it always sends the same values. If you need true closed-loop, build the emulator separately.

<a id="part-13"></a>
## Part 13: Step-by-Step Vivado Implementation Guide

This is the original 17-step guide, now expanded with explanations. Follow in order.

### Step 1 — Create Vivado Project Targeting KR260

1. Open Vivado.
2. **Create Project** → next.
3. Name your project (e.g., `kr260_aurora_test`).
4. Project type: **RTL Project**, "Do not specify sources at this time".
5. Default Part: click **Boards** tab → search "KR260" → select the SOM + carrier card.
6. Finish.

**Why:** Selecting the board (not just the chip) gives you board files with pre-defined pin maps for SFP+, GTH ref clocks, etc.

### Step 2 — Create the Pattern Generator RTL File

1. In Flow Navigator → **PROJECT MANAGER** → **Add Sources** → **Add or Create Design Sources** → next.
2. **Create File** → File type: Verilog, File name: `axis_pattern_generator.v`.
3. Finish. Vivado opens the new file.
4. Paste the Verilog code from [Part 5.4](#part-5).
5. Save.

### Step 3 — Package the RTL as a Reusable IP

1. Tools → **Create and Package New IP** → Next.
2. Choose **Package your current project** → Next.
3. Set IP location (default fine) → Next → Finish.
4. The packager opens. Verify:
   - **Compatibility:** check "Zynq UltraScale+" is included.
   - **Customization Parameters:** see `PACKET_LENGTH` and `INTER_PKT_GAP` are exposed.
   - **Ports and Interfaces:** the `m_axis_*` signals should be auto-grouped under one interface called `m_axis` of type `xilinx.com:interface:axis_rtl:1.0`.
5. **Review and Package** → click "Package IP".
6. Close the packager.

**Why:** Packaging makes the RTL block reusable in IP Integrator (Block Design) — you can drag it like any other Xilinx IP.

### Step 4 — Verify AXI Stream Interface Naming

In the IP packager's "Ports and Interfaces" tab, you should see:
- An interface called `m_axis` (NOT individual signals).
- Inside it, the standard AXIS signals: `TDATA`, `TVALID`, `TREADY`, `TLAST`.

If Vivado did NOT auto-group them, your port names are wrong. Go back to Step 2 and fix the names to match exactly: `m_axis_tdata`, `m_axis_tvalid`, `m_axis_tready`, `m_axis_tlast`.

### Step 5 — Create a Block Design

1. Flow Navigator → **IP Integrator** → **Create Block Design**.
2. Name: `system`. OK.
3. Empty BD canvas opens.

### Step 6 — Add Required IPs

In the BD canvas, click "Add IP" (the "+" icon) and add each of these one at a time:

| IP Name | Search term | Configuration after adding |
|---------|-------------|----------------------------|
| Zynq UltraScale+ MPSoC | "zynq" | Run Block Automation; enable PL Clock 0 (100 MHz); enable PL Reset 0 |
| `axis_pattern_generator` | (the IP you just packaged) | Default config |
| AXI4-Stream Data FIFO | "axis data fifo" | Set "Independent Clocks" if crossing clock domains |
| Aurora 8B/10B | "aurora 8b10b" | Configure to match `top.xsa/design_1.hwh`: AXI4-Stream user interface, 1 lane, 4-byte lane width, 2 Gbps line rate, 125 MHz refclk, 50 MHz init clock, Framing mode, Duplex, Flow Mode = None, GT channel X0Y4 (Quad X0Y1). See §5 of `docs/milestone2/M2-A_auroralink_findings.md` for the complete checklist. |
| AXI4-Stream FIFO (PG080) | "axi-stream fifo" (NOTE: the one with AXI-Lite control, NOT the simpler axis Data FIFO) | Default; ensure "Use TX Data" is checked if you need TX too |
| ILA | "system ila" | Number of probes ~12, depth 1024 |
| Processor System Reset | "proc sys reset" | One per clock domain — add 2 |

> **Important:** There are TWO different "FIFO" IPs in Xilinx:
> - **AXI4-Stream Data FIFO** = simple, no AXI-Lite, no interrupt. Use for inter-block buffering (e.g., between PatternGen and Aurora).
> - **AXI4-Stream FIFO (PG080)** = full-featured, has AXI-Lite control, has interrupt output. Use for the RX path that the CPU drains.

### Step 7 — Connect Pattern Generator to TX FIFO

In BD canvas:
1. Click on `axis_pattern_generator/m_axis` port → drag a wire to `axis_data_fifo/S_AXIS`.

Vivado may auto-route. Verify there's a clean connection.

### Step 8 — Connect TX FIFO to Aurora TX

1. Click on `axis_data_fifo/M_AXIS` → drag to `aurora_8b10b/S_AXI_TX`.

### Step 9 — Configure Aurora Loopback (PMA Near-End)

Two ways:

**Method A — Hard-coded:** In the Aurora IP customization (double-click), find the "Loopback" parameter and set to "Near-End PMA Loopback". Re-customize.

**Method B — Pin-driven:** Leave loopback parameter at default. Then:
- Add a "Constant" IP from the IP catalog. Configure it: width=3, value=2 (binary 010 = PMA loopback).
- Connect Constant.dout → `aurora_8b10b/loopback`.

> Method B is preferred — easy to switch off later when connecting real fiber.

### Step 10 — Add the RX FIFO (PG080)

1. The RX FIFO IP should already be in the BD from Step 6.
2. Connect `aurora_8b10b/M_AXI_RX` → `axi_fifo_mm_s/AXI_STR_RXD` (the AXI Stream RX input on PG080 FIFO).

### Step 11 — Wire RX FIFO to PS for Software Access

The PG080 FIFO has THREE interfaces:
- `S_AXI` — AXI-Lite control register interface (CPU reads/writes via `XLlFifo_*` driver)
- `AXI_STR_RXD` — RX data stream from Aurora
- `interrupt` — output to GIC

Connections:
1. `axi_fifo_mm_s/S_AXI` → connect to PS `M_AXI_HPM0_LPD` (or any AXI master from PS) via an `axi_smartconnect` block.
2. `axi_fifo_mm_s/interrupt` → connect to PS `pl_ps_irq0[0]` (or any free PL→PS interrupt).
3. `axi_fifo_mm_s/AXI_STR_RXD` → from Aurora's M_AXI_RX (already done in Step 10).

Run **Connection Automation** to wire AXI-Lite paths automatically.

### Step 12 — Clock Connections (Detailed)

Refer back to [Part 9](#part-9). Summary of the connections:

| Source clock | Destination |
|--------------|-------------|
| `Zynq.pl_clk0` | Pattern Generator `aclk` |
| `Zynq.pl_clk0` | TX Data FIFO `s_axis_aclk` |
| Aurora `user_clk_out` | TX Data FIFO `m_axis_aclk` (clock crossing!) |
| Aurora `user_clk_out` | Aurora `s_axi_tx_aclk` (the IP's own AXIS TX clock; check the IP — sometimes it's the same wire) |
| Aurora `user_clk_out` | RX FIFO PG080 `axis_aclk` (the streaming side) |
| `Zynq.pl_clk0` | RX FIFO PG080 `s_axi_aclk` (the AXI-Lite control side) |
| `Zynq.pl_clk0` | All `proc_sys_reset.slowest_sync_clk` (for the PS-clock domain instance) |
| Aurora `user_clk_out` | The second `proc_sys_reset.slowest_sync_clk` (for the user-clock domain instance) |
| External **125 MHz** from KR260 board (pins Y6/Y5) | Aurora `gt_refclk1_p/n` (board flow connects automatically if you mapped Aurora to the KR260 SFP+ board interface; the frequency comes from `top.xsa/design_1.hwh` `C_REFCLK_FREQUENCY=125`) |

### Step 13 — Reset Connections

| `proc_sys_reset` instance | Outputs go to |
|---------------------------|---------------|
| `rst_pl_clk0` | `axis_pattern_generator/aresetn`, `axis_data_fifo/s_axis_aresetn`, `axi_fifo_mm_s/s_axi_aresetn`, AXI-Lite interconnect |
| `rst_aurora_user` | `axis_data_fifo/m_axis_aresetn`, Aurora's `s_axi_tx_aresetn`, `axi_fifo_mm_s/s_axi_aclk_2_aresetn` (whatever it's called for the streaming side) |

Both `proc_sys_reset` instances take `Zynq.pl_resetn0` as `ext_reset_in`.

### Step 14 — ILA Probes

Wire ILA probe inputs to:
- probe0 (32 bits) = `aurora_8b10b/m_axi_rx_tdata`
- probe1 (1 bit) = `aurora_8b10b/m_axi_rx_tvalid`
- probe2 (1 bit) = `aurora_8b10b/m_axi_rx_tlast`
- probe3 (1 bit) = `aurora_8b10b/channel_up`
- probe4 (1 bit) = `axi_fifo_mm_s/interrupt`
- ... (add more as needed from [Part 10.4](#part-10))

Set ILA's `clk` to whichever clock the probed signals run on (Aurora `user_clk` for the AXI Stream signals).

### Step 15 — Validate Design

1. **Tools → Validate Design** (or F6).
2. Fix any errors. Common issues:
   - Floating clock or reset → unconnected pin warning. Connect it.
   - Address map missing → run "Assign Address" (toolbar button) for AXI-Lite slaves.
   - Clock domain mismatch on FIFO → make sure FIFO is configured for "Independent Clocks" if its input/output clocks differ.

### Step 16 — Generate HDL Wrapper and Bitstream

1. In the Sources panel → right-click your BD → **Create HDL Wrapper** → Let Vivado manage → OK.
2. Set the wrapper as top (right-click → "Set as Top").
3. Run **Synthesis → Implementation → Generate Bitstream**.
4. Wait (5–30 minutes depending on PC).
5. After bitstream succeeds, **File → Export → Export Hardware** → include bitstream → save XSA.

### Step 17 — Hardware Debug

1. Connect KR260 via USB-UART and via JTAG (or use single-cable JTAG-over-USB on KR260).
2. Open Vivado **Hardware Manager** (Flow Navigator).
3. Auto-connect to the device.
4. Right-click the FPGA target → Program Device → choose your `.bit` file.
5. ILA core should auto-detect. Set up trigger (e.g., `m_axi_rx_tlast == 1`).
6. **Run Trigger Immediate** to capture.
7. Verify:
   - `channel_up = 1`
   - `m_axi_rx_tvalid` toggles
   - `m_axi_rx_tdata` shows your expected pattern (1.5 = 0x3FC00000, etc.)
   - `m_axi_rx_tlast = 1` once every 37 cycles in the active stream

<a id="part-14"></a>
## Part 14: Bring-Up and Testing on KR260 (from scratch)

### 14.1 Prerequisites Checklist

Before powering on the board:

- [ ] KR260 board with a known-good power supply
- [ ] USB-UART cable connected (for `xil_printf` console)
- [ ] JTAG cable (or USB-JTAG via the carrier card)
- [ ] Vivado **2022.1 or later** installed (the supplied `top.xsa` was generated with Vivado 2022.1 — `xsa.xml` records `Version="2022.1"`. Newer Vivado versions can usually re-elaborate but watch for IP-catalog version skew).
- [ ] Vitis IDE 2022.1 (or matching Vivado version) installed
- [ ] (For external loopback only) An SFP+ optical transceiver and a fiber jumper

> **Important scope note.** Part 14 below describes the **Vitis bare-metal** bring-up path — the same path the supplied CHIL reference (`xllfifo_interrupt_example_1`) uses. This is the *fastest* way to verify the Aurora link itself, because it avoids the Linux kernel and UIO layer entirely. The production protocol translator, however, will run on **Ubuntu on A53**; the equivalent Ubuntu-based test path is in `docs/milestone2/M2-A_auroralink_findings.md` §11 (`fpgautil` + `/dev/uio0` + `poll()`). Both paths are valid: bare-metal for first-light, Ubuntu for production.

### 14.2 Phase A — FPGA Bitstream Bring-Up

**Goal:** Verify the FPGA design loads and Aurora link comes up.

1. Power on KR260.
2. Open Vivado Hardware Manager → connect.
3. Program the bitstream you generated.
4. Open ILA, look for `channel_up` signal:
   - Expected: rises to `1` within ~1 second after programming.
   - If stuck at 0 → check Aurora reference clock, loopback configuration, reset sequence.

### 14.3 Phase B — Verify Pattern Generator Output

**Goal:** Confirm the pattern generator produces 37-word packets.

1. With ILA still running, set trigger: `m_axis_tlast == 1` on the pattern generator output.
2. Run trigger.
3. Inspect waveform:
   - `m_axis_tvalid` should be high during the 37-word sequence.
   - `m_axis_tdata` should show the expected hex values per word.
   - `m_axis_tlast` should be high only on word 37.

### 14.4 Phase C — Verify Loopback Round Trip

**Goal:** Confirm data sent on TX comes back on RX correctly.

1. Set ILA trigger: `m_axi_rx_tlast == 1` on Aurora RX output.
2. Run trigger.
3. Inspect:
   - 37 words appear on `m_axi_rx_tdata`.
   - Values match what you sent on `m_axis_tdata`.
   - `m_axi_rx_tlast` high on word 37.

If values are corrupted → loopback or Aurora encoding is broken. If values are right → physical link works.

### 14.5 Phase D — Software Bring-Up

**Goal:** A53 receives interrupt and processes the packet.

1. In Vitis, create an application project from the XSA.
2. Use the existing `main.c`, `Axi_IO.c`, and the rest of the PEBB CHIL source tree.
3. Build → produces `.elf`.
4. Open Vitis debugger, set breakpoints in:
   - `FifoHandler` (line 428 of main.c)
   - `axiReceive` (line 604 of Axi_IO.c)
5. Run on hardware. Observe:
   - Breakpoint in `FifoHandler` hits within ~10 ms after FPGA programming.
   - In `axiReceive`, step through the 37-word loop. Check `RxWord` matches your test pattern.
6. Print `PEBB_Control_U.feedback[0]` after `axiReceive` returns. Should equal 1500.0 (= 1.5 * 1000).

### 14.6 Phase E — Watch the Controller Run

**Goal:** PEBB controller computes outputs.

1. Add a `xil_printf` after `PEBB_Control_step()` in `runControlCycle()`:
```c
xil_printf("switch_1[0] = %f\r\n", PEBB_Control_Y.switch_1[0]);
```
2. Rebuild, rerun.
3. UART should show non-zero duty cycles after a few cycles.

### 14.7 Phase F — Watch TX

**Goal:** Verify A53 sends 23 words.

1. ILA trigger on `s_axi_tx_tlast == 1` (Aurora TX side).
2. Run.
3. Verify 23 words go out. Their content reflects controller output.

### 14.8 Phase G — Multi-Packet Stress Test

**Goal:** Ensure system handles back-to-back packets at full rate.

1. Set `INTER_PKT_GAP` parameter low (e.g., 10000 = ~100 µs at 100 MHz, slightly slower than RTDS).
2. Run for 60 seconds.
3. Check UART for:
   - "Length mismatch" warnings → if any, link is dropping data.
   - `commGlitchCtr` value via debugger → should stay at 0 or increment very slowly.
   - `intOverrunFlag` → should be 0.
4. If no warnings → ramp INTER_PKT_GAP down further to test margin.

### 14.9 Common Failure Modes and Fixes

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `channel_up` stays 0 | Reference clock missing, or no loopback enabled | Probe the ref clock with ILA; verify `loopback=3'b010` |
| Data on RX is all zeros | Aurora not receiving anything | Check loopback wiring; check that pattern gen is producing data (probe `m_axis_*`) |
| Data corrupted (random values) | Clock domain crossing not handled | Verify TX FIFO is configured "Independent Clocks"; verify each block uses correct `aclk` |
| FIFO interrupt never fires | Interrupt not connected to PS, or `XLlFifo_IntEnable` not called in software | Check BD wiring of `interrupt` to `pl_ps_irq*`; verify line 281 in main.c is reached |
| "Length mismatch" warnings constantly | Pattern gen is sending wrong number of words | Recheck Verilog; ensure `tlast` only on word 37 |
| `CHIL_error` set immediately | Likely NaN in float words | Replace any 0xFFFF... or unusual hex with valid IEEE 754 floats |
| Controller doesn't run | `run` flag is 0 | Verify word 35 in pattern gen is `0x00000001`; verify `ctrlOn=1` (set via UART) |
| UART silent | UART not initialized, or wrong baud rate | Check setupUART() runs first; baud is 115200 by default |

<a id="part-15"></a>
## Part 15: Q&A — Every Question You Will Face

### Q1: Counter generator banana hai — kya complex hai?

**Answer:** Not complex. It's about ~80 lines of Verilog. Use the template in [Part 5.4](#part-5). Three things to remember:
1. 37 words per packet (parameter PACKET_LENGTH).
2. TLAST only on the 37th word.
3. INTER_PKT_GAP for spacing between packets.

### Q2: `m_axis` kya matlab hai exactly?

**Answer:**
- `m_axis` = **M**aster **AXI S**tream — your block PRODUCES (outputs) data.
- The signals inside this group: `tdata`, `tvalid`, `tready`, `tlast`.
- Master output → connects to a Slave input (`s_axis_*`) on the next block.
- Naming MUST be exactly `m_axis_tdata`, etc. — Vivado uses these names to auto-detect the AXI Stream interface.
- See [Part 4](#part-4).

### Q3: Debug zaroori hai ya skip kar sakte hain?

**Answer:** ILA is **strongly recommended** during bring-up — it's how you SEE what's happening on FPGA wires. After everything works, you can remove ILA from the final bitstream to save FPGA area. See [Part 10](#part-10).

For first-time bring-up: include ILA. There is no other way to verify channel_up, AXI Stream signals, or interrupt firing without it (other than blind UART logging which is much slower).

### Q4: Transmitter to Receiver data kaise pass karte hain?

**Answer:** Through AXI Stream. Specifically:

1. Pattern Gen's `m_axis` (master output) connects to TX FIFO's `s_axis` (slave input).
2. TX FIFO's `m_axis` connects to Aurora's `s_axi_tx`.
3. Aurora encodes and sends on the GTH transceiver (TX serial pins).
4. (Loopback or fiber) — physical layer.
5. Aurora's `m_axi_rx` (after decoding) connects to RX FIFO's `s_axis_rxd`.
6. RX FIFO buffers, fires interrupt to PS.
7. CPU reads via memory-mapped registers using `XLlFifo_*` driver functions.

It's a chain of AXI Stream connections + Aurora encoding/decoding + a memory-mapped final stage.

### Q5: TX kaunse pin pe connect hota hai?

**Answer:** TX goes to one of the **GTH transceiver TX pins**, which on KR260 are physically routed to the SFP+ cage connector. Specifically:
- SFP+ cage 0 uses `MGTH_TX0_P/N` (one of the 4 channels in GTH Quad).
- SFP+ cage 1 uses a different channel.

You don't manually pick the pin — when you choose KR260 as the board target in Vivado, the board file maps Aurora to the correct SFP+ connector. See [Part 7](#part-7).

### Q6: Loopback ki tarah hota hai ya koi aur way?

**Answer:** Three options, pick based on what you have:

1. **Internal PMA loopback** — wires inside the GTH transceiver, no external hardware. Best for first bring-up. Set `loopback=3'b010`.
2. **External fiber loopback** — short fiber cable from SFP+0 TX to SFP+0 RX (or to SFP+1) on the same board. Tests physical layer. Set `loopback=3'b000`.
3. **Real link** — fiber to RTDS or another endpoint.

See [Part 8](#part-8).

### Q7: Step 9 (clock and reset) ka detailed explanation?

**Answer:** See [Part 9](#part-9). Summary:
- Multiple clock domains exist (for **this** design, per `design_1.hwh`): PS clock 100 MHz (pl_clk0), Aurora `user_clk_out` 50 MHz, GT reference clock 125 MHz, Aurora init clock 50 MHz.
- Each block must use the right clock for its signals.
- Use TWO `proc_sys_reset` instances — one per clock domain.
- The TX FIFO between PatternGen and Aurora must be configured "Independent Clocks" because it crosses domains.

### Q8: ILA kyu chahiye?

**Answer:** Without ILA, you have NO visibility into what's happening on the FPGA wires. You can't tell if Aurora is up, if data is flowing, or if interrupts are firing. ILA = oscilloscope inside the chip. See [Part 10](#part-10).

### Q9: Counter ka output 37 words ka packet hota hai — kya yeh AuroraLink format mein hai?

**Answer:** No, your Verilog only produces **AXI Stream** (TDATA + TVALID + TLAST). The Aurora IP wraps that in AuroraLink/64B-66B format on the wire. So:
- You write AXI Stream → Aurora makes it AuroraLink → over the wire → Aurora unwraps → AXI Stream comes out at the other end.

Your counter just needs to produce clean 37-word AXI Stream packets. Aurora handles the rest. See [Part 6](#part-6).

### Q10: Counter generator should generate data verifiable by Axi_IO.c — how?

**Answer:** Refer to the table in [Part 5.2](#part-5) which lists every word's expected type. Then use the Verilog template in [Part 5.4](#part-5) which has all the IEEE 754 hex codes pre-computed.

Key requirements:
- Word 35 = 1 (run flag).
- Floats should be valid IEEE 754 (not NaN).
- Word 37 = sequence number 1..1000.
- Sequence number increments per packet.

Software's `axiReceive()` will then:
- Parse 37 words.
- Apply scale factors (1000 for kA→A, 1e6 for MW→W).
- Store in `PEBB_Control_U` and other globals.
- `runControlCycle()` will compute outputs and call `axiSend()`.

### Q11: Buffer/FIFO — agar data already hai aur next set aata hai aur previous abhi tak read nahi hua, kya hoga?

**Answer:** The PG080 RX FIFO holds **multiple complete packets**. When new data arrives:
- If FIFO has space → new words are stored after the existing ones. Each packet's length is tracked separately.
- Software reads `XLlFifo_iRxGetLen()` → returns length of CURRENT (oldest) packet only.
- Software drains it with N calls to `XLlFifo_RxGetWord()`.
- Then `iRxGetLen()` returns next packet's length, and so on.
- If FIFO is FULL → backpressure deasserts `tready`, which propagates back through Aurora to the source. In loopback, pattern gen pauses safely. With real RTDS, data may be dropped (link error).

See [Part 11](#part-11) for full backpressure analysis.

### Q12: Closed-loop cycle kaise complete kare — counter → controller → switches → wapas?

**Answer:** Pure PMA loopback does NOT close the controller loop. The controller's 23-word TX would loop back as a 23-word RX, which causes a length mismatch error (37 expected, 23 received).

Three workarounds:
1. **Decoupled testing (recommended):** Set `ctrlOn=0` so the controller doesn't TX. Pattern gen → RX → axiReceive verifies reception. Then enable controller and observe TX via ILA only.
2. **Two Aurora links:** Pattern gen on Aurora 0, controller TX on Aurora 1. No interference.
3. **RTDS Emulator block:** Custom Verilog that listens to controller TX, simulates state, generates next 37-word packet. True closed-loop but complex.

See [Part 8.6](#part-8) and [Part 12](#part-12).

### Q13: Kria board lend hua. Project from scratch start kaise karu?

**Answer:**
1. Install Vivado + Vitis (matching version, e.g., 2022.2).
2. Download Kria KR260 board files (auto-installs with Vivado, or via Xilinx board store).
3. Create new Vivado project, target KR260 board.
4. Follow [Part 13](#part-13) Steps 1–17 to build the FPGA design.
5. Generate bitstream, export XSA.
6. Open Vitis, create application project from XSA. Add the existing `src/` directory of the PEBB CHIL firmware.
7. Build, debug.
8. Follow [Part 14](#part-14) bring-up phases A through G.

### Q14: SFP+ optical module zaroori hai?

**Answer:** Only for external fiber loopback or real RTDS link. For internal PMA loopback testing (recommended first phase), no SFP+ module is needed at all. The TX/RX never leaves the FPGA chip.

### Q15: Aurora line rate kya set karu?

**Answer:** Use **2 Gbps**. This is **not** a free choice — it is what the existing RTDS CHIL reference bitstream (`top.xsa/design_1.hwh`) uses (`C_LINE_RATE=2`, `C_REFCLK_FREQUENCY=125`, `aurora_8b10b` PG046). Going to 5/10 Gbps would require switching to PG074 (64B/66B), changing refclk to 156.25 MHz, and would silently not interop with the RTDS partner. For lab testing with internal PMA loopback, the rate just has to be self-consistent with the refclk, but staying at the production 2 Gbps means the lab bitstream and the field bitstream are the same artefact.

### Q16: Pattern generator `INTER_PKT_GAP` kya rakhu shuruaat mein?

**Answer:** Start very slow (1 packet per second = `INTER_PKT_GAP = 100_000_000`). This gives you time to read UART output, single-step in debugger, and observe ILA waveforms without overwhelming yourself. Once everything works, ramp down toward the assumed RTDS-realistic 125 µs (`INTER_PKT_GAP = 12_500`); confirm the exact value against RTDS docs or measured inter-RC-interrupt period before claiming "matches RTDS rate".

### Q17: `xil_printf` doesn't work — what's wrong?

**Answer:** Common causes:
- `setupUART()` not called before any `xil_printf`. It's called at line 178 of `main.c` — make sure that path runs.
- UART pins not exposed in BD. Verify Zynq IP has UART0/UART1 enabled and routed.
- Wrong baud rate on host PC terminal. Default = 115200 8N1.
- Wrong COM port. Use Device Manager (Windows) to identify the USB-UART COM port.

### Q18: Kya hum software update kiye bina counter ka pattern change kar sakte?

**Answer:** Yes. The counter's payload values are stored in a Verilog ROM (`payload[]`). To change values:
1. Edit the `initial begin` block in `axis_pattern_generator.v`.
2. Re-run synthesis + implementation + bitstream.
3. Reprogram FPGA (no software rebuild needed).

For runtime-changeable patterns, you'd need to add an AXI-Lite slave register to your generator IP and write to it from software. That's a future enhancement.

### Q19: Multi-packet test — kaise verify karu controller correctly chal raha?

**Answer:** Add `xil_printf` in `runControlCycle()` after `PEBB_Control_step()`:
```c
xil_printf("seq=%d, run=%d, switch_1[0]=%f, feedback[0]=%f\r\n",
           seq, run, PEBB_Control_Y.switch_1[0], PEBB_Control_U.feedback[0]);
```
This prints once per cycle. With pattern gen sending fixed values and incrementing `seq`, you'll see:
- `seq` incrementing 1, 2, 3, ...
- `feedback[0]` always = 1500.0 (the value we sent).
- `switch_1[0]` should be a non-zero duty cycle once controller is `enabled`.

### Q20: Verilog mein floating-point math zaroori hai?

**Answer:** No — and you should AVOID it. Pre-compute all your float values in Python (or a hex calculator), put them as `32'h....` constants in your Verilog ROM. Doing real-time float math in FPGA fabric requires DSP/FP IP cores, which is overkill for a static test pattern. See [Part 5.3](#part-5).

### Q21: KR260 carrier card different SOMs ko support karta hai — kya difference padta hai?

**Answer:** KR260 is specifically designed for K26 SOM. The exact UltraScale+ part is `XCK26-SFVC784-2LV-C`. Vivado's KR260 board file targets this part. Don't substitute K24 or other SOMs without rebuilding board file.

### Q22: Reset signal high or low — kaunsa active?

**Answer:**
- Most AXI signals use **active-LOW** reset, denoted by trailing `n`: `aresetn`. (1 = normal operation, 0 = reset)
- Some IPs (older Aurora flavors, some debug IPs) use **active-HIGH** reset.
- Always check the IP documentation. The `proc_sys_reset` IP outputs both polarities (`peripheral_aresetn` is active-low, `peripheral_reset` is active-high).

### Q23: Power consumption / thermal concerns on KR260?

**Answer:** GTH transceivers consume noticeable power (~0.5 W per active channel). For loopback testing it's not a concern. For multi-Aurora-link production setups, consider thermal management — the KR260 has a fan that should be enabled in the Zynq config.

### Q24: Vitis se build error: "undefined reference to XLlFifo_*"

**Answer:** Add the `axi4-stream-fifo` (or `xllfifo` library — exact name varies) to your BSP. In Vitis: open BSP settings → Libraries → enable `xllfifo`. Re-build BSP, then re-build app.

### Q25: Aurora "channel_up = 0" forever — kya troubleshoot karu?

**Answer:** Step-by-step:
1. Probe `gt_pll_lock` with ILA. Should be 1. If 0 → reference clock not present or wrong frequency. For this design, `gt_refclk1_p/n` must be wired to the KR260 **125 MHz** differential pair on **Y6/Y5** (see `design_1.hwh`). With KR260 board files installed, the board flow makes this connection automatically.
2. Probe `pma_init`, `reset_pb`. Both should be 0 (deasserted) after init. If stuck high → reset connection broken.
3. Set `loopback = 3'b010` (PMA loopback). If `channel_up` rises → physical fiber/SFP is the problem. If still 0 → IP configuration issue.
4. Open Aurora IP customization, verify line rate, lane count, reference clock frequency match your design.
5. Make sure `init_clk` is connected and ticking.

### Q26: Software stuck in `while(1)` — interrupt never fires. Kya gadbad?

**Answer:** Step-by-step:
1. Probe `interrupt` output of RX FIFO with ILA. Does it pulse?
   - Yes: hardware fires interrupt, but software not receiving. Check IRQ wiring to PS, check `XScuGic_Connect()` succeeded, check `Xil_ExceptionEnable()` was called.
   - No: interrupt isn't generated. Check that data with TLAST=1 actually reaches RX FIFO. Verify `XLlFifo_IntEnable(InstancePtr, XLLF_INT_ALL_MASK)` is called (line 281 of main.c).
2. Verify the GIC interrupt ID matches what's exported from the BD. In Vitis, check `xparameters.h` for `XPAR_*_INTR` macro and ensure `FIFO_INTR_ID` in `main.c` matches.

### Q27: Sequence number wrap (1000 → 1) edge case

**Answer:** When `seq` wraps from 1000 back to 1, the software's `SEQ_INCREMENT` check (if enabled) handles wraparound via `% SEQ_MAX`. In our default configuration, `SEQ_INCREMENT` is commented out (Axi_IO.h line 30), so no check is performed. Just keep the counter wrapping properly.

### Q28: Mujhe Verilog testbench likhna hai pattern generator ke liye?

**Answer:** Strongly recommended. A small testbench like this lets you simulate before going to hardware:

```verilog
module tb_pattern_gen;
    reg aclk = 0;
    reg aresetn = 0;
    wire [31:0] tdata;
    wire        tvalid, tlast;
    reg         tready = 1;

    axis_pattern_generator #(.INTER_PKT_GAP(100)) u (
        .aclk(aclk), .aresetn(aresetn),
        .m_axis_tdata(tdata),
        .m_axis_tvalid(tvalid),
        .m_axis_tready(tready),
        .m_axis_tlast(tlast)
    );

    always #5 aclk = ~aclk;  // 100 MHz

    initial begin
        #100 aresetn = 1;
        #100000 $finish;
    end

    always @(posedge aclk) begin
        if (tvalid && tready)
            $display("t=%t  word=%h  last=%b", $time, tdata, tlast);
    end
endmodule
```

Run in Vivado simulator → check the printed words match your expected pattern. Saves hours vs hardware-only debugging.

### Q29: Buffer size — kitni rakhu RX FIFO ki?

**Answer:** Default PG080 size is enough. Typically 512 to 4096 words. Our 1 packet is 148 bytes (37 words × 4) — well below any reasonable depth. If you expect software to lag (e.g., long debug breakpoints), increase to 4096 to buffer more packets without dropping.

### Q30: Final checklist before declaring "system works":

- [ ] `channel_up = 1` consistently
- [ ] ILA shows 37 words received per RX packet, TLAST on word 37
- [ ] `commGlitchCtr` stays at 0 for at least 60 seconds at full rate
- [ ] `intOverrunFlag` never sets
- [ ] `feedback[0]` equals expected scaled value (1500.0 for 1.5 kA input)
- [ ] `controllerStarted = 1` and `PEBB_Control_step` runs each cycle
- [ ] TX FIFO sees 23 words exiting per controller cycle
- [ ] `CHIL_error == 0` (no errors flagged)
- [ ] No NaN warnings in UART log
- [ ] Sequence number `seq` increments correctly

If all check, link + software + controller are working end-to-end. Ready for real RTDS connection.

---

## Glossary

| Term | Meaning |
|------|---------|
| AXI Stream | Internal-FPGA simple data streaming protocol with TDATA, TVALID, TREADY, TLAST |
| Aurora | Xilinx serial-link IP family. Two variants: **8B/10B** (PG046, up to ~6.6 Gbps) — used in this design — and **64B/66B** (PG074, higher rates). Specific configuration for this project: `aurora_8b10b` v11.1, 2 Gbps, 125 MHz refclk, 1 lane (per `top.xsa/design_1.hwh`). |
| BD | Block Design (Vivado IP Integrator) |
| BSP | Board Support Package (drivers + headers used in Vitis app) |
| CHIL | Controller Hardware-In-the-Loop |
| FIFO | First-In-First-Out buffer |
| GIC | General Interrupt Controller (in PS) |
| GTH | Multi-gigabit serial transceiver in Zynq UltraScale+ |
| ILA | Integrated Logic Analyzer (Xilinx debug IP) |
| IP | Intellectual Property block (a reusable hardware module) |
| KR260 | Kria Robotics 260 board (K26 SOM + carrier) |
| Loopback | Routing TX output back to RX input (internal or external) |
| PG080 | Xilinx Product Guide 080 — AXI4-Stream FIFO |
| PL | Programmable Logic (FPGA fabric) |
| PMA | Physical Medium Attachment (lower-level analog/serializer block) |
| PS | Processing System (the A53 + cortex-R5 + peripherals on Zynq) |
| RC | Receive Complete (interrupt flag in PG080 FIFO) |
| RTDS | Real-Time Digital Simulator |
| SFP+ | Small Form-factor Pluggable Plus — fiber optic connector standard |
| SOM | System-on-Module |
| XSA | Xilinx Shell Archive (hardware export from Vivado for Vitis) |

---

*End of guide. For software-side / firmware-side details (`main.c`, `Axi_IO.c` flow), see `detailed_explaination.md`.*
