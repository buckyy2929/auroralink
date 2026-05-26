# M2-A — AuroraLink Protocol & IP-Core Behavior

**Status:** Complete  
**Sources:** `RTDS_Aurora_Link/Kria_CHIL_Firmware/PEBB_CHIL/top.xsa` (hardware handoff `design_1.hwh`), `xllfifo_interrupt_example_1/src/main.c`, `xllfifo_interrupt_example_1/src/Axi_IO.c`, `Axi_IO.h`

---

## 1. Who Sends Data — The RTDS Simulator

The Aurora link partner is an **RTDS real-time digital simulator**. RTDS drives the Aurora TX line into the Kria K26 at a fixed control frame rate. The Kria receives simulation state (voltages, currents, commands) and sends back controller outputs (switching signals, status) each cycle.

This is not a general-purpose streaming link — it is a **cyclic, frame-synchronous exchange** driven by the RTDS timestep.

---

## 2. Aurora Variant — Confirmed from Hardware Export

**IP:** `aurora_8b10b` v11.1 — Xilinx PG046  
**Not** Aurora 64b/66b (PG074).

Source: `design_1.hwh` extracted from `top.xsa`

| Parameter | Value | Notes |
|---|---|---|
| `C_AURORA_LANES` | 1 | Single serial lane |
| `C_LANE_WIDTH` | 4 bytes | 32-bit AXI-Stream data path |
| `C_LINE_RATE` | 2 Gbps | Serial line rate |
| `C_REFCLK_FREQUENCY` | 125 MHz | GT reference clock (board-supplied) |
| `C_INIT_CLK` | 50 MHz | Initialization clock |
| `user_clk_out` | 50 MHz | AXI-Stream clock driven into FIFO and PS |
| `Interface_Mode` | Framing | Packet-framed; `tlast` signals frame end |
| `Dataflow_Config` | Duplex | Bidirectional TX and RX |
| `Flow_Mode` | None | No NFC or UFC flow control |
| `Backchannel_mode` | Sidebands | |
| GT type | GTH | Zynq UltraScale+ high-speed transceiver |
| GT channel | X0Y4 (Quad X0Y1) | Physical transceiver location on KR260 |
| Board target | `kr260_som` | Kria KR260 SOM — `xck26`, package `sfvc784`, `-2LV` |

**Clock math check:** 2 Gbps × (8/10 encoding) = 1.6 Gbps usable ÷ 32 bits = **50 MHz user clock** ✓

---

## 3. Push vs Pull — Answered

> **RX path: PL stream-push into AXI FIFO → hardware interrupt notification to PS → CPU word-pull from AXI FIFO.**

Three distinct steps, each owned by a different layer:

| Step | Owner | Mechanism |
|---|---|---|
| Aurora RX → FIFO | PL (hardware) | Aurora IP pushes AXI-Stream beats into AXI FIFO automatically when link delivers data (`TVALID`/`TREADY`). No CPU involved. |
| Frame-complete notification | PL → PS | AXI FIFO asserts `interrupt` line → wired to `pl_ps_irq0` on PS GIC → fires `XPAR_FABRIC_LLFIFO_0_VEC_ID` |
| Data retrieval | PS (CPU) | Software calls `XLlFifo_RxGetWord()` in a loop — explicit AXI reads, one 32-bit word per call |

Source: `design_1.hwh` (`axi_fifo_mm_s_0_interrupt` → `zynq_ultra_ps_e_0.pl_ps_irq0`), `main.c` (`SetupInterruptSystem` using `XScuGic`).

---

## 4. Frame Boundary Signaling

**Framing mode** (`Interface_Mode = Framing`) means the Aurora IP asserts `tlast` on the AXI-Stream output at the end of each packet. The AXI FIFO captures this and raises the **RC (Receive Complete)** interrupt bit (`XLLF_INT_RC_MASK`) when a full frame has landed.

Call chain in CHIL reference:

```
XScuGic IRQ fires
  └─ FifoHandler()                   [main.c]
       └─ checks XLLF_INT_RC_MASK
            └─ FifoRecvHandler()     [main.c]
                 └─ axiReceive()     [Axi_IO.c]
                      ├─ XLlFifo_iRxGetLen()   — reads frame length in bytes
                      ├─ length / WORD_SIZE     — converts to word count
                      └─ XLlFifo_RxGetWord()   — pulls each 32-bit word in a loop
```

The CHIL reference validates frame length before processing:
```c
if (ReceiveLength != RECEIVE_LENGTH) {
    // drain FIFO, discard, increment glitch counter
}
```

---

## 5. IP Configuration Checklist (Synthesis Parameters)

When synthesizing a new bitstream for the protocol translator, replicate these exact settings in the Vivado Aurora 8b/10b IP customization dialog:

| Parameter | Required Value | Where to Set |
|---|---|---|
| IP variant | `aurora_8b10b` (PG046) | IP Catalog selection |
| Lanes | 1 | Lane Selection tab |
| Lane width | 4 bytes (32-bit) | Lane Selection tab |
| Line rate | 2 Gbps | Line Rate field |
| GT reference clock | 125 MHz | Clocking tab |
| Init clock | 50 MHz | Clocking tab |
| Interface mode | Framing | Flow Control tab |
| Dataflow | Duplex | General tab |
| Flow control | None | Flow Control tab |
| GT type | GTH | Auto-selected for xck26 |
| GT channel | X0Y4 | Physical Implementation tab |
| Board | `kr260_som` | Board tab |

AXI FIFO IP (`axi_fifo_mm_s` v4.2): data width 32-bit, interrupt enabled, connected to `pl_ps_irq0`.

---

## 6. PS-Side Interface Pattern (From CHIL Reference)

The CHIL reference firmware implements the Aurora adapter as a **flat C module** (`Axi_IO.c`). The pattern is:

- A static array of `axiData` structs defines the field map — one entry per word in the frame
- Each entry carries: a pointer to the destination variable, wire type (float32/s32), local type (double/s32), and a scale factor
- `axiReceive()` loops over the array, reads one word per iteration, type-converts, scales, and writes to the target

```c
typedef struct {
    void*   ptr;          // destination variable address
    type_   localType;    // how to store locally (double_, s32_, ...)
    type_   axiType;      // wire format from RTDS (float_, s32_, ...)
    float   scaleFactor;  // e.g. 1000.0 to convert kV → V
} axiData;
```

In the CHIL reference, this is **hardcoded** (37 RX words, 23 TX words, all field pointers compiled in).

---

## 7. Key Design Decision — Configurable Field Map

The protocol translator must **not** hardcode the frame layout. The message format (number of words, field types, scale factors, names) is not yet defined and will likely change across integrations.

The Aurora adapter module must load its field map from a configuration file at startup:

```yaml
aurora_rx:
  word_count: 37          # total words expected per frame
  fields:
    - name: voltage_high
      word_index: 1
      wire_type: float32
      local_type: double
      scale: 1000.0
    - name: run_cmd
      word_index: 0
      wire_type: s32
      local_type: s32
      scale: 1.0
aurora_tx:
  word_count: 23
  fields:
    - ...
```

At runtime, the adapter builds its RX and TX tables from this file, validates incoming frame length against `word_count`, and writes decoded values into the translation layer's shared buffer by field name — not by hardcoded pointer.

---

## 8. Next Steps

| # | Action | Owner | Depends On |
|---|---|---|---|
| 1 | Open `top.xsa` block design in Vivado, verify Aurora IP settings against this table | Hardware engineer | This doc |
| 2 | Instantiate Aurora 8b/10b (PG046) in new Vivado project for the translator bitstream using the parameters in §5 | Hardware engineer | §5 checklist |
| 3 | Wire Aurora AXI-Stream output → `axi_fifo_mm_s`, interrupt → `pl_ps_irq0`, validate in block design | Hardware engineer | Step 2 |
| 4 | Define the YAML/JSON config schema for the configurable field map (§7) | Software engineer | §7 design decision |
| 5 | Implement `aurora_adapter` PS module: config loader, IRQ handler, RX/TX loops using `XLlFifo_*` | Software engineer | Steps 3, 4 |
| 6 | Obtain RTDS AuroraLink manual section specifying their 2 Gbps / 8b10b configuration — confirm interoperability parameters | Team | RTDS access |
| 7 | Hardware loopback test: Aurora IP in loopback mode, verify frame boundary (`tlast`) and RC interrupt fire correctly | Hardware + Software | Steps 3, 5 |

---

## 9. Aurora IP Core Synthesis & Kria Loopback Test

This section describes how to synthesize a standalone Aurora 8b/10b bitstream using Vivado in batch (terminal) mode and validate it on the Kria board with a loopback test — no RTDS simulator required.

Supporting files (created alongside this document):
- `scripts/vivado/build_aurora_loopback.tcl` — batch synthesis script
- `scripts/vivado/aurora_loopback.xdc` — pin constraints

---

### 9.1 IP Core Availability — No Separate Download Needed

`aurora_8b10b` v11.1 is **part of the Vivado IP catalog** — it is not separately downloadable. Once Vivado is installed, the IP is available immediately under IP Catalog → `Vivado Repository → Communication & Networking → Aurora 8B10B`.

**Vivado version required:** 2022.1 or later (KR260 board files first appeared in 2022.1).  
**License:** Aurora 8b/10b is a **No-Charge IP** for Zynq UltraScale+ devices including `xck26`. No separate license purchase needed — verify by opening IP Catalog and checking the license column shows "Included" for `aurora_8b10b`.

Install checklist:
```
☐ Vivado ML Edition 2022.1+  (free download from AMD/Xilinx downloads page)
☐ KR260 board files installed  (Tools → Vivado Store → Boards, search "kr260")
☐ aurora_8b10b shows "Included" in IP Catalog license column
```

---

### 9.2 Batch Synthesis — No GUI Required

Vivado supports a fully headless batch mode driven by a TCL script:

```bash
vivado -mode batch -source scripts/vivado/build_aurora_loopback.tcl
```

This runs synthesis, implementation, and bitstream generation without opening the GUI. Logs land in `vivado_project/aurora_loopback.runs/`.

Expected runtime on a modern workstation: **30–60 minutes** (dominated by implementation and routing).

---

### 9.3 Block Design — What the TCL Script Builds

The script (`scripts/vivado/build_aurora_loopback.tcl`) instantiates this block design:

```
[PS (zynq_ultra_ps_e)]
    pl_clk0 (100 MHz) ──► [clk_wiz] ──► 50 MHz ──► aurora init_clk
    M_AXI_HPM0_FPD   ──► [axi_interconnect] ──► [axi_fifo_mm_s]
    pl_ps_irq0        ◄── axi_fifo_mm_s.interrupt

[aurora_8b10b_0]
    user_clk_out (50 MHz) ──► axi_fifo_mm_s.s_axi_aclk
                          ──► axi_interconnect.ACLK
    M_AXI_RX (AXI-Stream RX) ──► axi_fifo_mm_s.AXI_STR_RXD
    S_AXI_TX (AXI-Stream TX) ◄── axi_fifo_mm_s.AXI_STR_TXD
    gt_refclk1_p/n    ◄── board 125 MHz diff pair (Y6/Y5)
    txp/txn, rxp/rxn  ──► GT serial I/O (looped externally or internally)
```

**Internal loopback for standalone test:** The script ties the Aurora `loopback` port to `3'b001` (near-end PCS loopback) so TX data is fed directly back into RX within the GTH transceiver — no fiber cable or SFP loopback plug required.

To switch to real RTDS operation, remove the loopback tie and connect the physical SFP port.

---

### 9.4 Key TCL Excerpt — Aurora IP Parameters

```tcl
create_bd_cell -type ip -vlnv xilinx.com:ip:aurora_8b10b:11.1 aurora_8b10b_0
set_property -dict [list \
    CONFIG.C_AURORA_LANES     {1}         \
    CONFIG.C_LANE_WIDTH       {4}         \
    CONFIG.C_LINE_RATE        {2}         \
    CONFIG.C_REFCLK_FREQUENCY {125}       \
    CONFIG.C_INIT_CLK         {50.0}      \
    CONFIG.DRP_FREQ           {50.0}      \
    CONFIG.Interface_Mode     {Framing}   \
    CONFIG.Dataflow_Config    {Duplex}    \
    CONFIG.Flow_Mode          {None}      \
    CONFIG.Backchannel_mode   {Sidebands} \
    CONFIG.CHANNEL_ENABLE     {X0Y4}      \
    CONFIG.C_START_QUAD       {Quad_X0Y1} \
    CONFIG.C_START_LANE       {X0Y4}      \
] [get_bd_cells aurora_8b10b_0]

# Near-end PCS loopback — remove for production
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 loopback_const
set_property CONFIG.CONST_VAL {1} [get_bd_cells loopback_const]
set_property CONFIG.CONST_WIDTH {3} [get_bd_cells loopback_const]
connect_bd_net [get_bd_pins loopback_const/dout] \
               [get_bd_pins aurora_8b10b_0/loopback]
```

See `scripts/vivado/build_aurora_loopback.tcl` for the complete script including wrapper generation, constraints, and bitstream export.

---

### 9.5 XDC Pin Constraints

```xdc
# GT 125 MHz reference clock — differential pair on KR260
# Source: design_1.hwh C_REFCLK_LOC_P=Y6, C_REFCLK_LOC_N=Y5
set_property PACKAGE_PIN Y6 [get_ports GT_125_REFCLK_clk_p]
set_property PACKAGE_PIN Y5 [get_ports GT_125_REFCLK_clk_n]
create_clock -period 8.000 -name gt_refclk [get_ports GT_125_REFCLK_clk_p]
```

See `scripts/vivado/aurora_loopback.xdc` for the full constraints file.

---

### 9.6 Loading the Bitstream on Kria (Ubuntu)

Kria running Ubuntu uses the Linux **FPGA Manager** subsystem. No reflashing — the bitstream loads at runtime.

**Step 1 — Copy artifacts to Kria:**
```bash
scp out/vivado/design_1.bit  ubuntu@<kria-ip>:/lib/firmware/
scp out/vivado/design_1.dtbo ubuntu@<kria-ip>:/lib/firmware/
```

**Step 2 — Load bitstream:**
```bash
ssh ubuntu@<kria-ip>
sudo fpgautil -b /lib/firmware/design_1.bit -o /lib/firmware/design_1.dtbo
```

**Step 3 — Verify:**
```bash
cat /sys/class/fpga_manager/fpga0/state
# Expected: "operating"
```

**Device Tree Overlay (design_1.dtbo):** Describes the AXI FIFO peripheral to the Linux kernel — its MMIO base address and interrupt number. Generated by Vitis from the exported XSA, or hand-written from the address map in `design_1.hwh`. This is a required step; without it, the UIO driver cannot see the FIFO.

---

### 9.7 Loopback Test Strategy

With the bitstream loaded and loopback enabled in silicon, the test confirms:
1. Aurora IP initialises and reaches `channel_up` state
2. AXI FIFO interrupt fires correctly after a TX → loopback → RX cycle
3. Data read back from RX FIFO matches data written to TX FIFO
4. Frame boundary (`tlast`) is correctly preserved through the loopback

**Test program approach (PS-side Linux):**

Access the AXI FIFO via `/dev/uio0` (registered by the device tree overlay) using `mmap`:

```c
// Open UIO device
int fd = open("/dev/uio0", O_RDWR);
void *base = mmap(NULL, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

// Write test frame to TX FIFO (N words)
uint32_t *fifo = (uint32_t*)base;
// ... write words via FIFO TX data register (TDFR offset)
// ... write frame length to TX length register (TLR offset)

// Wait for RX Complete interrupt via poll() on /dev/uio0
// Read back from RX FIFO and verify
```

**Pass criteria:**
- `channel_up` port asserted within 2 seconds of bitstream load
- RC interrupt fires within 1 ms of TX write completing
- RX word count matches TX word count
- All RX words match TX words bit-for-bit

**Failure modes to watch for:**
| Symptom | Likely cause |
|---|---|
| `channel_up` never asserts | GT loopback not wired, or init clock missing |
| RC interrupt never fires | FIFO interrupt not reaching PS GIC — check DTB IRQ number |
| RX length = 0 | `tlast` not asserted by Aurora in loopback — check framing mode config |
| Data corruption | Endianness mismatch or cache coherency issue on FIFO MMIO region |

---

*Sources: `top.xsa/design_1.hwh` (Aurora IP instance `aurora_8b10b_0`, `axi_fifo_mm_s_0`); `RTDS_Aurora_Link/Kria_CHIL_Firmware/PEBB_CHIL/xllfifo_interrupt_example_1/src/main.c`; `src/Axi_IO.c`; `src/Axi_IO.h`*
