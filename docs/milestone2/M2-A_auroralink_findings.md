# M2-A — AuroraLink Protocol & IP-Core Behavior

**Status:** Protocol & IP behaviour characterised against primary sources; on-board validation pending hardware + Vivado access. Open items tracked in §10.  
**Sources:** `RTDS_Aurora_Link/Kria_CHIL_Firmware/PEBB_CHIL/top.xsa` (hardware handoff `design_1.hwh`), `xllfifo_interrupt_example_1/src/main.c`, `xllfifo_interrupt_example_1/src/Axi_IO.c`, `Axi_IO.h`, PG046 (`aurora_8b10b`), PG080 (`axi_fifo_mm_s`), XTP685 (KR260 SOM XDC), XTP743 (KR260 carrier schematic), `kr260_som:1.0` and `kr260_carrier:1.0` board files. Verification batch in §0 below.

**Scope note (relative to `MILESTONE_2_TASKS.md` M2-A spec).** The spec calls for a ≤ 3-page findings document. The Executive Summary below is the ≤ 3-page deliverable. Everything from §0 onward is the supporting evidence pack (source citations, port tables, register maps, gap analysis, on-board checklist, fallback verification ladder for the no-RTDS / no-CHIL-carrier lab). The evidence pack is intentionally long because the supplied bitstream is from a different carrier card than the lab hardware — see §10.8 — and the reviewer will ask "how do you know?" on every parameter; §0–§9 answer that without paraphrase.

**Out-of-scope clarification.** The PS-side `aurora_adapter` (`bridge/src/aurora_adapter.c`) is **not** an M2-A deliverable; it is the M2-D bridge component. M2-A defines its required interface (frame size, IRQ semantics, byte order, type-conversion contract — see §§3–7 below); M2-D implements it. The header `bridge/include/aurora_adapter.h` exists today only as a design-time interface declaration; the `.c` file is a skeleton with `NOT_IMPLEMENTED`. The reviewer should not read M2-A as claiming the adapter is built.

---

## Executive Summary (the ≤ 3-page deliverable)

**What M2-A answers (the four questions in `M2-A_TASK.md`).**

1. **Variant / wire format.** Aurora 8B/10B v11.1 (Xilinx PG046), 1 lane × 4-byte AXI-Stream, **2 Gbps line rate**, **125 MHz GT refclk**, 50 MHz `user_clk_out`, Framing mode, Duplex, Flow Mode = None, GT channel X0Y4 in Quad X0Y1, refclk pins Y6/Y5 (= SOM240_2 C3/C4). Source: `design_1.hwh` extracted from supplied `top.xsa`. Not Aurora 64b/66b (PG074). Clock math consistency check passes only for 8B/10B (§2). **`loopback[2:0]` is hidden in this customisation** — it is tied internally to `3'b000` and does NOT appear at the IP wrapper top; this is a non-trivial point because it invalidates any "drive a constant on the wrapper" strategy for PMA loopback (§2.2, Gap 10.7c).
2. **Push or pull.** Three-step pipeline. **Push** in PL (Aurora streams beats into AXI FIFO automatically via TVALID/TREADY); **notification** by hardware interrupt (FIFO's `interrupt` line → `pl_ps_irq0` → GIC); **pull** in PS (`XLlFifo_RxGetWord()` loop). Same vocabulary as `BRIDGE_SOFTWARE_DESIGN.md` §2.3 (§3).
3. **Frame boundary.** Aurora IP asserts `tlast` on the AXI-Stream output at end-of-packet; AXI FIFO captures it and raises the **Receive Complete** interrupt (`XLLF_INT_RC_MASK = 0x04000000`, bit 26 in ISR). Userspace polls `/dev/uio0`, reads ISR/RDFO/RLR/RDFD, **must** `write(uio_fd, &one_u32, 4)` to re-arm UIO before the next `poll()` (§4, §11.4).
4. **Frame rate ("125 µs / 8 kHz").** Driven by the RTDS power-system model, NOT by Aurora. Unsourced in this repo — flagged Gap 10.2; resolution path is either an RTDS docs citation or a measured `inter-RC` time on live hardware (§§1, 11.5).

**Open items the reviewer will ask about (all numbered in §10).**

| # | Item | Status |
|---|---|---|
| 10.1 | RTDS-side Aurora docs section not cited | Open — awaiting vendor docs |
| 10.2 | "8 kHz / 125 µs" frame rate unsourced | Open — closes via §11.5 board measurement or RTDS docs |
| 10.3 | A53 on-board smoke test | Open — gated on board access |
| 10.4 | Vivado run / DTBO generation | Open — Vivado not on current build host; scripts are committed sketches |
| 10.5 | `aurora_adapter` implementation | Explicitly M2-D scope, not M2-A |
| 10.6 | DTBO not in repo | Open — produced by 10.4 |
| 10.7 | `build_aurora_loopback.tcl` is a minimal sketch with four named deviations from production | Open — handed off with line-by-line caveats |
| 10.8 | Stock KR260 carrier delivers **156.25 MHz** on the GT refclk pins, but the supplied bitstream expects **125 MHz** (XTP743 p.16 vs `design_1.hwh`) | **Resolution proposed (lab-variant rebuild at 156.25 MHz / 2.5 Gbps), pending Vivado validation — see §12.6 and 10.8 below** |

**On Gap 10.8 specifically — honest framing (this replaces an earlier overclaim).** Earlier drafts of this doc and the related Hindi/explainer files marked 10.8 as "RESOLVED via lab-variant rebuild". That was overstated. The accurate status is:

- The **architectural argument** for the lab variant is sound (§12.6): line rate stays at refclk × 16 for both 125 MHz / 2 Gbps and 156.25 MHz / 2.5 Gbps, so the GTH PLL ratio is preserved at the level of refclk-to-line-rate; the user clock derivation is the same 8/10 ÷ 32 = ×0.25 ratio; and every downstream stage (FIFO offsets, RC IRQ mask, framing/TLAST, UIO, libEGD ABI) is frequency-independent by construction.
- However, **2.5 Gbps is not one of the Aurora 8B/10B IP wizard's standard preset line rates** (the wizard's published presets in PG046 are 0.5 / 1.0 / 1.25 / 2.0 / 3.125 / 5.0 / 6.25 Gbps). At 2.5 Gbps the wizard may require either a custom-rate entry that the IP supports (it does — `C_LINE_RATE` is a real number, not an enum) or, in the worst case, a tweak to the GT_DIFFCTRL / CPLL feedback dividers that the wizard computes. We have NOT yet elaborated the IP at 156.25 MHz / 2.5 Gbps in Vivado (Gap 10.4: Vivado not on this build host).
- **Therefore the honest position is "resolution proposed and parametrised in `scripts/vivado/build_aurora_loopback.tcl`; pending validation in a Vivado run (closes when 10.4 runs)."** The parametrised TCL takes `--refclk_mhz` and `--line_rate_gbps`, so the same script also builds the production target (125 MHz / 2 Gbps) once a CHIL carrier or oscillator swap is available.
- We are **not** claiming the rebuild has been tested in silicon. We are claiming the lab variant is the architecturally minimal change set, fully parametrised in code, and ready to elaborate the moment Vivado is available.

**Verification strategy without RTDS hardware (the reviewer's first question).** Five-stage ladder (§12.2):

| Stage | What it proves | Hardware needed |
|---|---|---|
| S1 | Aurora IP customisation is internally consistent (RTL sim, PG046 testbench) | PC + Vivado |
| S2 | FIFO + IRQ + UIO + libEGD on real A53, Aurora bypassed | One KR260 |
| S3 | Aurora itself: `channel_up`, framing, TLAST survive a PMA loopback round-trip | One KR260 + rebuilt bitstream (lab-variant Aurora IP with `loopback[2:0]` exposed) |
| S4 (optional) | Adds physical SFP+ optical layer | + SFP+ module + LC-LC fibre |
| S5 (production) | RTDS-protocol interop | RTDS hardware (not in scope) |

The demo target is **S1 + S2 + S3**. Failure-mode catalog and prerequisites checklist are in §§12.3–12.4.

**M2-A complete, with the items in the table above carried forward as open.** Nothing in the protocol or IP characterisation is unknown; what is open is mechanical execution (Vivado run, board access, RTDS access) — handed off explicitly, not buried.

---

## 0. Verification Log

This section records which claims in this document are cross-verified against which authoritative source. Every numbered claim below is independently verifiable on the desk without a physical board.

| # | Claim | Verified against | Result |
|---|---|---|---|
| V1 | GT refclk pins are `Y6` (P) / `Y5` (N), Bank 224, MGTREFCLK0, routed to SOM240_2 pins C3/C4 | (a) `design_1.hwh` `C_REFCLK_LOC_P/N`; (b) **XTP685 `Kria_K26_SOM_Rev1.xdc` lines 284–285**; (c) `kr260_som/1.0/part0_pins.xml` lines 152–153 | **3 independent sources agree — double-confirmed** |
| V2 | KR260 board file does NOT define a `GT_125_REFCLK` board interface | `kr260_carrier/1.0/board.xml` defines only `sfp_led` (GPIO) and `sfp_iic` (I2C). No GTH refclk interface. | **TCL must use manual external ports + explicit XDC** (which our XDC already does; TCL needs the `apply_bd_automation` call removed — see Gap 10.7(b)) |
| V3 | `USER_DATA_S_AXIS_TX` IS the packaged bus-interface name in the Aurora IP wrapper; `s_axi_tx_*` are its constituent signals | PG046 §"Port Descriptions": *"USER_DATA_S_AXIS_TX is the interface and the s_axi_tx_* ports are grouped into that interface."* | **Earlier draft was wrong on this point — corrected.** `loopback[2:0]` IS a standard PG046 port but exposure at top-level depends on IP customisation (see Gap 10.7(c)). |
| V4 | All FIFO MMIO register offsets used in §11.4 (ISR=0x00, IER=0x04, TDFV=0x0C, TDFD=0x10, TLR=0x14, RDFO=0x1C, RDFD=0x20, RLR=0x24) and `XLLF_INT_RC_MASK = 0x04000000` (bit 26 = Receive Complete) | PG080 v4.2 §"Register Space" address-map table (pp. 65–66) + ISR bit definitions (pp. 69–71) | **All eight offsets and the RC bit position match exactly. No corrections needed.** |
| V5 | **`design_1.hwh` configures a 125 MHz refclk on SOM240_2 C3/C4** | **XTP743 (KR260 carrier schematic) page 16: the stock KR260 has a 156.25 MHz LVDS oscillator driving `GTH_REFCLK0_C2M_P/N` (= SOM240_2 C3/C4 = Y6/Y5). The stock carrier delivers 156.25 MHz, NOT 125 MHz.** Confirmed against the same schematic that nets `GTR_REFCLK0_C2M` (PS-side GTR) ARE driven by a 125 MHz oscillator (U87) but those are different pins (PS GTR, used for USB/GEM/DP), not the GTH SFP+ path. | **CRITICAL MISMATCH — see Gap 10.8.** The supplied bitstream is consistent with a **custom CHIL carrier card** (not the stock KR260 starter kit), OR with a hardware-modified stock KR260. Bringing the bitstream up on an unmodified stock KR260 will fail. |
| V6 | UG1092 (KR260 user guide) positions the SFP+ cage for 10G Ethernet (10GBase-SR pluggable, page 11 + Table 4) | UG1092 §"SFP+" + carrier-accessory table | **Consistent with V5.** 10G Ethernet line rate 10.3125 Gbps with 156.25 MHz refclk is the stock KR260's design intent. Aurora 2 Gbps with 125 MHz refclk is NOT what the stock KR260 carrier supports out of the box. |

**What changed in this revision (2026-05-27):**
- Gap 10.7 reduced from 5 deviations to 4 (V3 corrected the spurious bus-interface claim).
- Gap 10.7(b) sharpened: it is now a documented fact (V2) that the `apply_bd_automation` call in the TCL must be removed.
- **Gap 10.8 added (V5) — the highest-impact finding of this batch.** The reviewer will check this in 30 seconds; we name it before they ask.
- §11.4 FIFO register offsets re-confirmed against PG080 v4.2 (V4) — no changes needed.

---

## 1. Who Sends Data — The RTDS Simulator

The Aurora link partner is an **RTDS real-time digital simulator**. RTDS drives the Aurora TX line into the Kria K26 at a fixed control frame rate. The Kria receives simulation state (voltages, currents, commands) and sends back controller outputs (switching signals, status) each cycle.

This is not a general-purpose streaming link — it is a **cyclic, frame-synchronous exchange** driven by the RTDS timestep.

---

## 2. Aurora Variant — Confirmed from Hardware Export

**IP:** `aurora_8b10b` v11.1 — Xilinx PG046  
**Not** Aurora 64b/66b (PG074).

**Source:** `design_1.hwh` extracted from `top.xsa` (Vivado 2022.1, exported 2023-07-27).  
Extraction reproducible: `unzip -p top.xsa design_1.hwh`.

### 2.1 Verified parameter table

Every row below was read directly from the `aurora_8b10b_0` `<MODULE>` block in `design_1.hwh`.

| Parameter (in `design_1.hwh`) | Value | Notes |
|---|---|---|
| `VLNV` | `xilinx.com:ip:aurora_8b10b:11.1` | IP version |
| `C_AURORA_LANES` | `1` | Single serial lane |
| `C_LANE_WIDTH` | `4` | 4-byte (32-bit) AXI-Stream data path |
| `C_LINE_RATE` | `2` | 2 Gbps serial line rate |
| `C_REFCLK_FREQUENCY` | `125` | 125 MHz GT reference clock (board-supplied) |
| `C_INIT_CLK` | `50.0` | 50 MHz initialization clock |
| `DRP_FREQ` | `50.0000` | DRP clock frequency |
| `C_DRP_IF` | `false` | DRP wrapped internally; no external DRP interface to wire |
| `Interface_Mode` | `Framing` | Packet-framed; `tlast` signals frame end |
| `Dataflow_Config` | `Duplex` | Bidirectional TX and RX |
| `Flow_Mode` | `None` | No NFC or UFC flow control |
| `Backchannel_mode` | `Sidebands` | |
| `SINGLEEND_GTREFCLK` | `false` | Differential refclk |
| `C_REFCLK_LOC_P` | `Y6` | GT refclk + pin on KR260 package — XTP685 line 285 confirms `Y6 = MGTREFCLK0P_224 = som240_2_c3` |
| `C_REFCLK_LOC_N` | `Y5` | GT refclk − pin — XTP685 line 284 confirms `Y5 = MGTREFCLK0N_224 = som240_2_c4` |
| `C_START_QUAD` | `Quad_X0Y1` | GTH Quad selected |
| `C_START_LANE` / `CHANNEL_ENABLE` | `X0Y4` | Active GT channel in Quad |
| `C_REFCLK_SOURCE` | `X0Y4 clk0` | Refclk routing: clk0 of channel X0Y4 |
| `C_GT_CLOCK_1` | `GTHQ0` | GTH Quad 0 clock domain |
| Board target | `xilinx.com:kr260_som:1.0` | from `xsa.xml`; **NOT** `:part0:1.1` |
| Part | `xck26-sfvc784-2LV-c` | Kria K26 SOM |

**Clock math check:** 2 Gbps × (8/10 encoding) ÷ 32-bit data path = **50 MHz user clock** ✓ — matches the 50 MHz `user_clk_out` consumed by the AXI FIFO. (For 64b/66b: 2 × 64/66 / 32 = 60.6 MHz, which does not match the hwh; further independent proof the IP is 8B/10B.)

### 2.2 Top-level Aurora ports (as the wrapper actually exposes them)

This is the **exact port list** from `aurora_8b10b_0` in the hwh, not a generic PG046 example:

| Direction | Port | Used for |
|---|---|---|
| I | `s_axi_tx_tlast`, `s_axi_tx_tvalid` | TX user-data AXI-Stream signal-level pins |
| O | `s_axi_tx_tready` | TX backpressure |
| O | `m_axi_rx_tlast`, `m_axi_rx_tvalid` | RX user-data AXI-Stream signal-level pins |
| O | `channel_up` | Link-up status — use this for the §11.3 bring-up gate |
| O | `tx_lock` | PLL locked indicator |
| O | `hard_err`, `soft_err`, `frame_err` | Diagnostics (route to scratch register or ILA for bring-up) |
| O | `tx_resetdone_out`, `rx_resetdone_out`, `link_reset_out`, `pll_not_locked_out`, `sys_reset_out` | Reset/sequencing status |
| I | `reset`, `gt_reset`, `power_down` | Reset inputs |
| I | `gt_refclk1_p`, `gt_refclk1_n` | Differential refclk **signal-level** pins (NOT a packaged `GT_DIFF_REFCLK1` interface) |
| O | `gt_reset_out` | Reset-out from the IP |

The TX/RX user data widths flow through bus interfaces `aurora_8b10b_0_USER_DATA_M_AXI_RX` (RX) and the equivalent on TX. (Note the wrapper names use `AXI` rather than the conventional `AXIS`.)

**Important — the `loopback` GT control pin is NOT exposed at the top of this customisation.** PG046 supports loopback via a 3-bit control input (`3'b001` = near-end PCS, `3'b010` = near-end PMA), but to drive it from constants or PS-AXI scratch you must either (a) modify the IP customization to expose `loopback` as a top-level port, or (b) wrap the IP with thin RTL that ties `loopback` to a constant inside the wrapper. The minimal-loopback TCL in `scripts/vivado/build_aurora_loopback.tcl` cannot drive PCS loopback without this change. This is now Gap 10.7.

---

## 3. Push vs Pull — Answered

> **RX path: PL stream-push into AXI FIFO → hardware interrupt notification to PS → CPU word-pull from AXI FIFO.**

This is the exact vocabulary defined in [`BRIDGE_SOFTWARE_DESIGN.md` §2.3](BRIDGE_SOFTWARE_DESIGN.md): **stream producer (PL)** pushes beats, **delivery notification** is hardware interrupt, **data retrieval (PS)** is processor-initiated word pulls.

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

### 5.1 AXI FIFO IP — verified parameters from `axi_fifo_mm_s_0` in `design_1.hwh`

| Parameter | Value | Notes |
|---|---|---|
| VLNV | `xilinx.com:ip:axi_fifo_mm_s:4.2` | PG080 v4.2 |
| `C_TX_FIFO_DEPTH` | `512` words | |
| `C_RX_FIFO_DEPTH` | `512` words | |
| `C_RX_FIFO_PE_THRESHOLD` | `5` | RX programmable-empty watermark |
| `C_RX_FIFO_PF_THRESHOLD` | `507` | RX programmable-full watermark |
| `C_USE_TX_DATA` | `1` | TX data stream used |
| `C_USE_TX_CTRL` | `0` | TX control stream NOT used (single-stream config) |
| `C_USE_RX_DATA` | `1` | RX data stream used |
| `C_HAS_AXIS_TKEEP` | `1` | TKEEP signal present on AXI-Stream |
| `C_HAS_AXIS_TDEST` | `1` | TDEST present (routing tag) |
| `C_HAS_AXIS_TID` / `TSTRB` / `TUSER` | `0` | Not present |
| `C_BASEADDR` / `C_HIGHADDR` (AXI-Lite MMIO) | `0xA0010000` / `0xA001FFFF` | 64 KB register window |
| `C_AXI4_BASEADDR` / `C_AXI4_HIGHADDR` | `0x80001000` / `0x80002FFF` | Secondary AXI4-MM aperture (not used in interrupt-driven mode) |

### 5.2 IRQ wiring — verified from `design_1.hwh`

```
axi_fifo_mm_s_0.interrupt   (DIR=O, SENSITIVITY=LEVEL_HIGH, SIGIS=INTERRUPT)
        │  signal name: axi_fifo_mm_s_0_interrupt
        ▼
zynq_ultra_ps_e_0.pl_ps_irq0    (1-bit wide; PS configured: C_NUM_F2P_0_INTR_INPUTS=1)
```

**No `xlconcat` is used** — the production design configures the PS's `pl_ps_irq0` vector as 1-bit wide (`C_NUM_F2P_0_INTR_INPUTS=1`) and wires the single FIFO IRQ directly. When recreating this in any new TCL, the PS block must be customised with `PSU__USE__IRQ0=1` and the 1-bit input width, otherwise Vivado will either widen with implicit zero-padding (changes IRQ vector number visible in the DT) or refuse to connect.

### 5.3 PL clocks — verified

| Source | Frequency | Used by |
|---|---|---|
| `pl_clk0` (`PSU__CRL_APB__PL0_REF_CTRL__FREQMHZ=100`) | 100 MHz | PS-PL AXI control + interconnect |
| `pl_clk1` (`PSU__CRL_APB__PL1_REF_CTRL__FREQMHZ=100`) | 100 MHz | secondary PL domain in production design |
| `pl_clk2`, `pl_clk3` | disabled | |
| **External 25 MHz oscillator pin** | 25 MHz | Production design's `clk_wiz_0` `C_PRIM_IN_FREQ=25` input; clk_wiz outputs 50 MHz (init_clk for Aurora), 100 MHz, 30 MHz (ADC) |

**Bring-up TCL note:** the minimal loopback design in `scripts/vivado/build_aurora_loopback.tcl` derives Aurora's 50 MHz `init_clk` from `pl_clk0` (100 MHz) via a `clk_wiz`, rather than from the external 25 MHz oscillator that the production design uses. Functionally equivalent for loopback, but a different topology than production. See Gap 10.7.

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

Supporting files (committed to this repo alongside this document — see also §11 for the on-board verification checklist):
- [`scripts/vivado/build_aurora_loopback.tcl`](../scripts/vivado/build_aurora_loopback.tcl) — batch synthesis script
- [`scripts/vivado/aurora_loopback.xdc`](../scripts/vivado/aurora_loopback.xdc) — pin constraints

> **Status of these scripts:** committed and reviewed against `design_1.hwh`; **not yet executed end-to-end on a board** because the Kria hardware is not in hand at the time of writing. The first engineer with the board runs §11 checklist and records actual outputs.

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

## 10. Known Gaps / Open Items

These items are explicitly **not** answered by what is in this repo today. Each is a hand-off, not a hidden problem. The reviewer will ask about each one; the honest answer is here, not buried.

| # | Gap | Why it matters | What closes it |
|---|---|---|---|
| 10.1 | **No RTDS-side Aurora docs section cited.** The hardware-side parameters (variant, line rate, refclk, framing) are sourced from `top.xsa/design_1.hwh`. The RTDS partner's published configuration table is **not** in this repo. | If RTDS uses different lane count / line rate / framing, the link will not come up even though our bitstream is internally consistent. | Obtain the AuroraLink section of the RTDS technical manual; record the exact section URL + version in §2 of this doc; cross-check parameter table; mark this gap closed. |
| 10.2 | **Frame rate ("125 µs / 8 kHz") is unsourced.** Every reference to this number in `explaination_docs/` is hedged in the disclaimer at the top of each file, but the rate itself is set by the RTDS power-system model, not by Aurora. | If the model runs at, say, 50 µs, our software may overrun (`intOverrunFlag`) at the assumed 8 kHz budget. If it runs at 1 ms, we are massively over-provisioned. | (a) Cite the RTDS model docs, or (b) measure inter-RC-interrupt time on a live board (`channel_up` → first RC IRQ → next RC IRQ via `clock_gettime(CLOCK_MONOTONIC, …)`) and record. |
| 10.3 | **Smoke test on A53 hardware pending.** §9.7 describes the loopback test strategy; §11 below is the explicit checklist. No `channel_up`-on-silicon evidence exists yet. | Bitstream correctness, FIFO IRQ routing, libEGD ABI compatibility — all unverified on actual A53. | Execute §11 checklist; paste raw outputs (not paraphrased) back into this doc as Appendix A. |
| 10.4 | **Vivado availability.** The TCL/XDC scripts in `scripts/vivado/` are committed and reviewed against `design_1.hwh`, but Vivado is not installed on the current build host, so they have not been **elaborated or run** against the IP catalog. | A typo in a `set_property` would only surface at first synthesis attempt. | Run `vivado -mode batch -source scripts/vivado/build_aurora_loopback.tcl` on a workstation with Vivado 2022.1+ and KR260 board files; capture log; resolve any errors. |
| 10.5 | **Aurora adapter PS module is a stub.** `bridge/include/aurora_adapter.h` and `bridge/src/aurora_adapter.c` declare and skeleton-implement the runtime interface. Real config-driven RX/TX, IRQ delivery via UIO, and the field-map-driven type conversions are still M2-D scope. | The reviewer should not confuse "interface designed" with "module implemented". | Implement against the YAML schema in `config/aurora_fieldmap.example.yaml`; unit-test with simulated FIFO words on host; integration-test on board (§11.7). |
| 10.6 | **Linux UIO / kernel driver path is described, not delivered.** §9.6 shows the `fpgautil` + `mmap` + `poll()/dev/uio0` outline; the device-tree overlay (`design_1.dtbo`) is not in this repo (it is generated from the synthesized design). | Without the DTBO, `/dev/uio0` does not exist, and the `mmap` example does not run. | Generate DTBO from the synthesized output in step 10.4; commit it under `scripts/vivado/out/` (or document the regeneration command). |
| 10.7 | **`build_aurora_loopback.tcl` is a MINIMAL LOOPBACK SUBSET of the production design, with four named deviations** (revised 2026-05-27 after a second pass against `design_1.hwh` + PG046 + the KR260 board files). The script will not run unmodified — these are real issues, not paranoia: <br>(a) Board part string was `kr260_som:part0:1.1`; the actual hwh ships from board `xilinx.com:kr260_som:1.0`. Fixed in script but call out the version-sensitivity. <br>(b) **The KR260 SOM board file (`kr260_carrier:1.0/board.xml`) defines NO `GT_125_REFCLK` board interface** — only `sfp_led` (2-bit GPIO) and `sfp_iic` (I2C to SFP+ EEPROM). Any `apply_bd_automation -rule xilinx.com:bd_rule:board` against a refclk interface name **will fail**. The correct pattern is the one our XDC already uses: manual external port creation + explicit `PACKAGE_PIN Y6/Y5` constraints. The TCL still has an `apply_bd_automation` call that must be removed and replaced with `create_bd_port`. <br>(c) The Aurora `loopback[2:0]` control pin is documented in PG046 Table "User Interface Ports" as a standard IP port, but is NOT in the top-level port list of THIS particular customization (likely tied internally to `3'b000` by the IP wrapper). To run near-end PMA loopback the IP must be re-customised in the Vivado IP catalog to expose `loopback`. <br>(d) PS IRQ width must be set to `C_NUM_F2P_0_INTR_INPUTS=1` (production setting per hwh); the TCL currently does not set this, so the implicit 8-bit `pl_ps_irq0` vector will shift the IRQ number observed by the kernel and break the DTBO. <br>~~(e) Production design includes `au_drop_0`, `au_modify_0`, `aurora_reset_0`, BRAM scratch, three ILAs, three `proc_sys_reset` blocks, three CDC generators — the bring-up TCL deliberately omits all of these.~~ Acceptable for first-light, but the bitstream is NOT equivalent to production. <br><br>**CORRECTED from previous version:** an earlier draft asserted that `USER_DATA_S_AXIS_TX` was not a valid bus-interface name and that TX pins must be wired signal-level. PG046 (re-read 2026-05-27) clarifies that `USER_DATA_S_AXIS_TX` *is* the packaged bus-interface name in the IP wrapper and `s_axi_tx_*` are its constituent signals. Our TCL's `connect_bd_intf_net … USER_DATA_S_AXIS_TX` is therefore correct; the warning header in the TCL has been softened accordingly. | The reviewer who opens the TCL will spot every one of these in five minutes. | The minimal correct path: (a) open the actual Vivado block design (if `top.bd` is available, share it — see ask B6 in the regroup notes), (b) `write_bd_tcl` from there to capture the production design verbatim, (c) keep the current script ONLY as a starting-skeleton with explicit "expect manual fixes at first elaboration" caveats. Until then, treat the TCL as a checked sketch, not a runnable artefact. |
| **10.8** | **STOCK KR260 OSCILLATOR FREQUENCY MISMATCH — resolution proposed, pending Vivado validation.** Finding: supplied `design_1.hwh` configures Aurora for `C_REFCLK_FREQUENCY=125` MHz / `C_LINE_RATE=2` Gbps on GTH Quad X0Y1 MGTREFCLK0 (Y6/Y5 → SOM240_2 C3/C4). Per **XTP743 page 16** the stock KR260 carrier delivers **156.25 MHz** on those pins (the design-intent for the 10G-Ethernet SFP+ cage). The stock 125 MHz oscillator (U87) drives the PS-side GTR (USB / DisplayPort / GEM), NOT the GTH SFP+ path. **Proposed resolution (not yet validated in Vivado):** parametrise the bring-up build so that for lab verification on a stock KR260, the Aurora IP is rebuilt with `C_REFCLK_FREQUENCY=156.25` and `C_LINE_RATE=2.5` Gbps. The refclk-to-line-rate ratio stays at ×16, so the architectural relationship the GTH PLL must produce is identical; the user-clock derivation (line × 8/10 ÷ 32) gives 62.5 MHz instead of 50 MHz but every downstream block (TX AXIS FIFO already async, FIFO MMIO offsets, RC IRQ mask, framing/TLAST, UIO re-arm, libEGD ABI) is frequency-independent by construction. The parametrisation is committed in `scripts/vivado/build_aurora_loopback.tcl` (`--refclk_mhz`, `--line_rate_gbps`). **What is NOT yet proven, and which the reviewer is right to push on:** (a) that the Aurora IP wizard accepts 2.5 Gbps cleanly as `C_LINE_RATE` (it is not one of the wizard's standard published presets — see PG046 §"Customizing the IP"; the IP does accept arbitrary `C_LINE_RATE` values within the GTH range but the GT_DIFFCTRL / CPLL feedback dividers the wizard chooses must be reviewed); (b) that the GTH CPLL settings computed by the IP at 156.25 MHz / 2.5 Gbps actually train on real hardware. Both can only be confirmed by elaborating the IP in Vivado (Gap 10.4) and then running stage S3. See §12.6 below for the full lab-variant parameter table; see §12.7 for the position statement. | The reviewer might check this in 30 seconds; we name the mismatch, name the proposed resolution, and clearly distinguish "architecturally argued" from "Vivado-validated" — the latter is what we still owe. | (a) Elaborate `scripts/vivado/build_aurora_loopback.tcl` at the default 156.25 / 2.5 parameters; confirm `report_design_analysis` shows GTH CPLL settings within Vivado's recommended range; (b) run S3 (PMA loopback) on a stock KR260; (c) for production switch to 2 Gbps / 125 MHz refclk: same script with `--refclk_mhz 125 --line_rate_gbps 2`, plus physical hardware with a 125 MHz oscillator (custom CHIL carrier OR oscillator swap on stock). No software, RTL pattern, or driver changes between lab variant and production. |

---

## 11. When the Board Arrives — Verification Checklist

This is the **mechanical checklist** for the first engineer who plugs in a KR260. Do not paraphrase results; paste the raw command output into Appendix A of this doc (create one when the first run happens).

### 11.1 Pre-flight (host side, before powering the board)

```bash
# Vivado 2022.1+ with KR260 board files installed
vivado -version | head -1                              # expect: Vivado ML 2022.1 or later
vivado -mode batch -source scripts/vivado/build_aurora_loopback.tcl 2>&1 | tee out/vivado/build.log
ls -la out/vivado/design_1.bit out/vivado/design_1.dtbo
# Expect both files present; .bit ~ a few MB, .dtbo ~ a few KB.
```

**Pass:** TCL exits with status 0; both artifacts exist.
**If fail:** read `out/vivado/build.log` for the first `ERROR:` line; gap 10.4 is closed only when this runs clean.

### 11.2 Bitstream load on Kria (Ubuntu)

```bash
# Copy artifacts
scp out/vivado/design_1.bit  ubuntu@<kria-ip>:/lib/firmware/
scp out/vivado/design_1.dtbo ubuntu@<kria-ip>:/lib/firmware/

# Load
ssh ubuntu@<kria-ip> 'sudo fpgautil -b /lib/firmware/design_1.bit \
                                    -o /lib/firmware/design_1.dtbo'

# Verify
ssh ubuntu@<kria-ip> 'cat /sys/class/fpga_manager/fpga0/state'
# Expect:  operating

ssh ubuntu@<kria-ip> 'ls -la /dev/uio*'
# Expect:  /dev/uio0  (one entry per AXI FIFO declared in the DTBO)
```

**Pass:** state = `operating`, `/dev/uio0` exists.

### 11.3 `channel_up` confirmation

The production hwh exposes `channel_up` only at the IP top-level, not in any AXI-Lite scratch register. Two options for the bring-up bitstream:

**Option A (recommended for bring-up): add a tiny `axi_gpio` slave** at MMIO `0xA002_0000` (next free aperture after the FIFO at `0xA0010000–0xA001FFFF`) and wire `aurora_8b10b_0.channel_up`, `tx_lock`, `hard_err`, `soft_err`, `frame_err` to GPIO inputs. Then:

```bash
ssh ubuntu@<kria-ip> 'sudo devmem 0xA0020000 32'
# Expect bit 0 set: channel_up = 1
# Expect bit 1 set: tx_lock = 1
# Bits 2,3,4 should be 0: hard_err / soft_err / frame_err
```

**Option B (zero-RTL): probe via ILA in Vivado Hardware Manager.** The production design already instantiates `ila_0`, `ila_1`, `system_ila_0` — the bring-up TCL must include the system_ila for this to work. See `KR260_auroralink.md` Part 10.

**Pass:** `channel_up = 1` within 2 seconds of bitstream load.
**If fail:** see failure modes table in §9.7 — most common cause is wrong refclk pin mapping or missing init clock.

### 11.4 Loopback RC interrupt round-trip

A minimal C program against `/dev/uio0` (or the EGD-style cross-compiled binary from M2-C, repurposed). All offsets below are PG080 v4.2 register offsets; the FIFO MMIO base is `0xA0010000` (verified from `design_1.hwh`).

```c
int fd = open("/dev/uio0", O_RDWR);
volatile uint32_t *fifo = mmap(NULL, 0x10000,
                               PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
/* PG080 v4.2 register offsets (verify against the IP's PG046 table) */
#define IER  0x04   /* Interrupt Enable */
#define ISR  0x00   /* Interrupt Status */
#define TDFV 0x0C   /* TX Data FIFO Vacancy */
#define TDFD 0x10   /* TX Data FIFO Data */
#define TLR  0x14   /* TX Length */
#define RDFO 0x1C   /* RX Data FIFO Occupancy */
#define RDFD 0x20   /* RX Data FIFO Data */
#define RLR  0x24   /* RX Length */

fifo[IER/4] = 0x04000000;          /* RC IRQ enable (XLLF_INT_RC_MASK) */

for (int i = 0; i < 4; i++) fifo[TDFD/4] = 0xDEADBEE0u | i;
fifo[TLR/4] = 4 * 4;               /* 4 words × 4 bytes = 16 bytes */

uint32_t enable = 1;
write(fd, &enable, sizeof(enable));   /* re-arm UIO interrupt */
struct pollfd p = { .fd = fd, .events = POLLIN };
poll(&p, 1, 1000);                   /* 1 s timeout */

uint32_t isr = fifo[ISR/4];
fifo[ISR/4] = isr;                   /* clear */
uint32_t n   = fifo[RDFO/4];
uint32_t len = fifo[RLR/4];
for (uint32_t i = 0; i < n; i++) printf("rx[%u] = 0x%08x\n", i, fifo[RDFD/4]);
```

**Pass:**
- `poll()` returns < 1 ms after the TX `TLR` write.
- `isr & 0x04000000` (`XLLF_INT_RC_MASK`) set.
- `n == 4`, `len == 16`, all four RX words match `0xDEADBEE0..0xDEADBEE3`.

**Critical detail many guides omit:** UIO clears the IRQ enable bit in the kernel after every wake; userspace MUST `write(fd, &one, 4)` to re-arm before the next `poll()`. Without this the second `poll()` blocks forever even though more IRQs arrive.

**If fail:** see §9.7 failure modes; most common is DTBO IRQ-number mismatch (RC IRQ wired in PL but not in DT, so UIO never wakes).

### 11.5 Frame-rate measurement (closes gap 10.2)

With the pattern generator from `KR260_auroralink.md` Part 5 driving the link at the production-realistic `INTER_PKT_GAP`, run:

```c
struct timespec t0, t1;
clock_gettime(CLOCK_MONOTONIC, &t0);
poll(/* /dev/uio0 */);   // wait for next RC IRQ
clock_gettime(CLOCK_MONOTONIC, &t1);
printf("inter-RC = %ld ns\n", (t1.tv_sec-t0.tv_sec)*1000000000L + (t1.tv_nsec-t0.tv_nsec));
```

Run for 10000 cycles, record mean / p99 / max.

**Pass:** mean within ±5 % of the RTDS model's documented step; p99 < mean + 50 µs.
**Output:** update §1 ("fixed control frame rate") to cite the measured value; close gap 10.2.

### 11.6 libEGD ABI smoke test (closes M2-C remaining item)

```bash
scp out/arm64/lib/libEGD.so   ubuntu@<kria-ip>:/tmp/
scp out/arm64/bin/JsonPublishC ubuntu@<kria-ip>:/tmp/

ssh ubuntu@<kria-ip> 'LD_LIBRARY_PATH=/tmp /tmp/JsonPublishC'
# Expected (no EGD config server reachable):  Error reading config(-1): NETWORK_ERROR
# That output proves the binary RAN — no SIGILL, no relocation error, no glibc mismatch.
```

**Pass:** any non-SIGILL non-relocation output.

### 11.7 Aurora adapter integration

With the stubs from `bridge/include/aurora_adapter.h` filled in (M2-D scope) and the field map at `config/aurora_fieldmap.example.yaml`:
- Load config, validate against the schema (37 RX words, 23 TX words to match `Axi_IO.h`).
- Receive one RC IRQ; decode 37 words; assert per-field values for a known pattern from the pattern generator.

**Pass:** decoded values match the pattern generator's hand-crafted floats (e.g., word 2 = `0x3FC00000` → decoded as 1.5 × 1000 = 1500.0).

---

## 12. Verification Strategy when RTDS hardware is unavailable

This project will demo without access to an RTDS simulator/hardware. This section is the canonical reference for what "verified" can and cannot mean in that scenario. The narrative explainer doc `explaination_docs/KR260_auroralink.md` §8 is the engineer-facing tutorial; this section is the reviewer-facing position.

### 12.1 What we are NOT trying to prove

- We are NOT claiming end-to-end interop with a real RTDS. That is Gap 10.1 (RTDS docs) + Gap 10.2 (frame rate) + a live cable, and it stays open until RTDS is available.
- We are NOT proving the supplied production bitstream is correct. That bitstream has at least two issues we have identified independently of any RTDS: the `loopback[2:0]` port is tied internally (V3), and the refclk frequency is 125 MHz against a stock-KR260 carrier that delivers 156.25 MHz (Gap 10.8). The supplied bitstream is therefore a build artefact for a *different target* (custom CHIL carrier) and not a verification vehicle for a lab on stock hardware.

### 12.2 What we ARE proving (the five-stage ladder)

| Stage | Name | Hardware | Proves | Hours |
|---|---|---|---|---|
| **S1** | Vivado RTL simulation (PG046 example testbench) | PC only | Aurora IP customisation is internally consistent — framing, clocks, TLAST, frame-check loop | 1–2 |
| **S2** | On-board: pattern generator → AXI FIFO → IRQ → PS software, Aurora **bypassed** | One KR260 | FIFO + IRQ wiring + DTBO + UIO + libEGD + `axiReceive` code path are correct, on real silicon, independently of Aurora | half day |
| **S3** | On-board: same as S2 but routed through Aurora in PMA Near-End Loopback | One KR260 + bitstream rebuilt with re-customised Aurora IP that exposes `loopback[2:0]` | Aurora IP comes up (`channel_up=1`), refclk works, framing+TLAST survive a SERDES round-trip | 1 day |
| **S4** (optional) | External SFP+ fibre loopback | One KR260 + SFP+ optical module + LC-LC fibre jumper | Adds physical/optical layer | half day |
| **S5** (production) | Real cable to RTDS | RTDS hardware | RTDS-protocol interop | n/a |

**Recommended scope for the demo: complete S1 + S2 + S3.** Defer S4 unless the SFP+ module/fibre are already on the bench. S5 is gated on RTDS availability and is out of scope.

### 12.3 What each pass / fail means for the reviewer

When you walk the reviewer through results, the precise language matters:

| Stage outcome | What you can truthfully say |
|---|---|
| **S1 passes** | "The Aurora IP, configured per the production hwh's parameters (1 lane × 4-byte × 2 Gbps × 125 MHz refclk × Framing × Duplex), passes the PG046 self-checking testbench. Two instances of this IP can train and exchange frames in simulation. The IP-level configuration is internally consistent." |
| **S2 passes** | "Independently of Aurora, the AXI4-Stream FIFO + PL→PS interrupt + Linux UIO + libEGD ABI + the receive code path are verified on Kria silicon. A 37-word frame injected at the FIFO input arrives at the application as 37 words with TLAST on word 37, the RC interrupt fires once per frame, the UIO re-arm path is exercised, and the per-word values match. The software stack is ready for any AXI-Stream source." |
| **S3 passes** | "The full Aurora data path — Pattern Gen → TX FIFO → Aurora TX → GTH PMA loopback → Aurora RX → RX FIFO → IRQ → PS — is verified on Kria silicon. `channel_up` asserts (`devmem` confirms bit 0 of the status GPIO), no `hard_err`/`soft_err`/`frame_err` are observed, the per-word RX pattern matches the per-word TX pattern, and the round-trip preserves TLAST. The Aurora layer is ready." |
| **S3 passes, S4 not run** | "Physical/optical layer testing requires an SFP+ optical module and is deferred. PMA-loopback covers everything in the FPGA up to but not including the SFP+ cage." |
| **S5 not run** | "RTDS-protocol interop testing requires RTDS access; cited in Gap 10.1. The above S1–S3 chain verifies everything on our side of the link." |

If any stage fails, the precise level-of-failure tells the reviewer which subsystem to look at — and that itself is a credible engineering position. Random failures with no localisation aren't.

### 12.4 Concrete prerequisites checklist before S3

Before running PMA loopback on the board, verify all of:

- [ ] **Refclk / line rate handled.** Default TCL build uses 156.25 MHz / 2.5 Gbps (stock KR260 lab variant — Gap 10.8 proposed resolution). For a custom CHIL carrier card with 125 MHz oscillator, re-run with `--refclk_mhz 125 --line_rate_gbps 2`. **This box is unchecked until the IP elaborates cleanly in Vivado at 156.25 MHz / 2.5 Gbps (Gap 10.4) and `report_design_analysis` confirms the GTH CPLL configuration is in the supported range** — see Gap 10.8 for the open sub-items.
- [ ] **Aurora IP re-customised** so that `loopback[2:0]` appears at the IP wrapper top-level (V3 fix). Confirm via `report_design_analysis -port_list aurora_8b10b_0` after IP regen.
- [ ] **`xlconstant` driving `loopback`** set to `CONST_VAL=2`, `CONST_WIDTH=3` (= `3'b010` = PMA Near-End). NOT 1 (= PCS, skips SERDES). NOT 4 (= Far-End, useless for unilateral).
- [ ] **`axi_gpio_status`** added at `0xA002_0000` carrying `channel_up`, `tx_lock`, `hard_err`, `soft_err`, `frame_err`. Without this F5 (silent failure: no link, blame falls on software) is essentially guaranteed.
- [ ] **TX AXI-Stream Data FIFO** set to "Clock Type: Independent Clocks" (asynchronous). `s_axis_aclk = pl_clk0` (100 MHz); `m_axis_aclk = aurora_8b10b_0/user_clk_out` (50 MHz). Common-clock mode here = silent data corruption (F6).
- [ ] **PS IRQ width** = `C_NUM_F2P_0_INTR_INPUTS=1`. Without this the IRQ number in the DTBO is wrong and UIO doesn't wake (F9 / Gap 10.7(d)).
- [ ] **Pattern generator's TLAST** verified in ILA pulsing on every 37th beat (F8).
- [ ] **UIO re-arm** in the software handler (the `write(uio_fd, &one_u32, 4)` line in §11.4) — without this you get exactly one interrupt then silence (F10).

If ANY box above is unchecked, do not attempt S3. The probability of debugging a silent failure cleanly is low.

### 12.5 What happens after the demo

For the reviewer / customer / project lead, the prioritised follow-ups are:

1. **Close Gap 10.1 / 10.2** by obtaining the RTDS hardware manual section that defines the AuroraLink configuration (variant, line rate, framing, frame rate).
2. **For production switch from lab-variant to production target:** Either obtain a custom CHIL carrier card with a 125 MHz oscillator on `GTH_REFCLK0_C2M`, OR perform an oscillator swap on the stock KR260. Then re-customise the Aurora IP with `C_REFCLK_FREQUENCY=125` and `C_LINE_RATE=2`. No software, RTL pattern, or driver changes are required (see §12.6).
3. **Obtain the original Vivado workspace** (`top.xpr` + `top.srcs/sources_1/bd/design_1/design_1.bd` + custom RTL `au_drop.v`, `au_modify.v`, `aurora_reset.v`) — without it the production-design IPs (`au_drop_0`, `au_modify_0`, `aurora_reset_0`) remain black boxes. The bring-up TCL we built is a sketch, not a production replacement.

These three items are the gate between "verified architecture" (which we will have after S1+S2+S3) and "verified end-to-end production system" (which we cannot get to without RTDS plus the original BD).

### 12.6 Lab-variant Aurora IP configuration (the Gap 10.8 resolution)

This is the **exact** parameter set we use for stage S3 on a stock KR260 starter kit. The only two values that change vs the production hwh are highlighted in **bold**; everything else is identical to §2.1 of this document.

| Parameter | Production target (per `design_1.hwh`) | **Lab variant on stock KR260** | GTH PLL math |
|---|---|---|---|
| `C_REFCLK_FREQUENCY` | 125 MHz | **156.25 MHz** | Physical oscillator value from XTP743 p.16 |
| `C_LINE_RATE` | 2 Gbps | **2.5 Gbps** | Both equal refclk × 16 — same integer PLL multiplier |
| `C_AURORA_LANES` | 1 | 1 | unchanged |
| `C_LANE_WIDTH` | 4 | 4 | unchanged |
| Aurora `user_clk_out` (derived) | 50 MHz (= 2 G × 8/10 ÷ 32) | 62.5 MHz (= 2.5 G × 8/10 ÷ 32) | IP computes automatically |
| `C_INIT_CLK` | 50 MHz | 50 MHz | unchanged — internal init |
| `Interface_Mode` | Framing | Framing | unchanged |
| `Dataflow_Config` | Duplex | Duplex | unchanged |
| `Flow_Mode` | None | None | unchanged |
| `Backchannel_mode` | Sidebands | Sidebands | unchanged |
| `SINGLEEND_GTREFCLK` | false | false | unchanged |
| `C_REFCLK_LOC_P` / `_N` | Y6 / Y5 | Y6 / Y5 | unchanged — same pins |
| `C_START_QUAD` / `CHANNEL_ENABLE` | X0Y1 / X0Y4 | X0Y1 / X0Y4 | unchanged |

**What this means in practice:**

1. **Software stack** (`aurora_adapter`, FIFO MMIO offsets, RC IRQ mask, UIO re-arm, libEGD ABI): bit-for-bit identical between lab variant and production. Anything you verify in software at 2.5 Gbps stays verified at 2 Gbps.
2. **FIFO + AXI-Stream framing**: identical. TLAST behaviour identical. PG080 register map identical.
3. **Clock-domain crossing on the TX AXIS Data FIFO**: the `m_axis_aclk` changes from 50 MHz to 62.5 MHz. The FIFO must already be in "Independent Clocks" (async) mode — it is, per §9.3 of `explaination_docs/KR260_auroralink.md`. So the FIFO copes with both frequencies transparently.
4. **Timing closure** in the bring-up bitstream: at 62.5 MHz the closure margin is *easier* than the production 100 MHz pl_clk0; no timing issues expected.
5. **Production switch**: just two parameter changes (`C_REFCLK_FREQUENCY = 125`, `C_LINE_RATE = 2`) + physical hardware with a 125 MHz oscillator. Rebuild bitstream. Everything else carries over.

**Build command (parametrised TCL):**

```bash
# Lab variant on stock KR260
vivado -mode batch \
       -source scripts/vivado/build_aurora_loopback.tcl \
       -tclargs --refclk_mhz 156.25 --line_rate_gbps 2.5

# Production target (after Gap 10.1 closes and the right carrier is available)
vivado -mode batch \
       -source scripts/vivado/build_aurora_loopback.tcl \
       -tclargs --refclk_mhz 125 --line_rate_gbps 2
```

The two refclk/line-rate values are the only thing the TCL needs from the operator; everything else is encoded in the script.

### 12.7 Position on Gap 10.8 — for the reviewer

> *The supplied bitstream is configured for a 125 MHz refclk that the stock KR260 carrier does not natively provide. Rather than gating S3 on custom hardware, we have parametrised the bring-up build so the Aurora IP can be rebuilt at 156.25 MHz / 2.5 Gbps as a lab variant — the same refclk × 16 line-rate relationship as the production target, with every downstream stage (FIFO, IRQ, software, framing) frequency-independent by construction. We have not yet elaborated the IP in Vivado at the lab-variant parameters; that, plus stage S3 on silicon, is what closes the gap. The production parameter switch (125 MHz / 2 Gbps) is the same script with two CLI arguments, once a CHIL carrier or oscillator-swap board is available.*

This is the honest position: **proposed and parametrised in code; pending two concrete validation steps** — (a) Vivado IP elaboration at 156.25 MHz / 2.5 Gbps reports a clean GTH CPLL configuration (`report_design_analysis`), (b) stage S3 PMA loopback on silicon shows `channel_up = 1` and a TLAST-clean RX pattern. Until both are done, calling 10.8 "resolved" is overreach; calling it "deferred" or "blocked" is also wrong because the resolution path is fully scripted. The accurate verb is **"resolution proposed, pending Vivado + board validation."**

---

*Sources: `top.xsa/design_1.hwh` (Aurora IP instance `aurora_8b10b_0`, `axi_fifo_mm_s_0`); `RTDS_Aurora_Link/Kria_CHIL_Firmware/PEBB_CHIL/xllfifo_interrupt_example_1/src/main.c`; `src/Axi_IO.c`; `src/Axi_IO.h`; `docs/BRIDGE_SOFTWARE_DESIGN.md` §2.3; PG046 §"Port Descriptions"; PG080 v4.2 register map; XTP685 `Kria_K26_SOM_Rev1.xdc`; XTP743 KR260 Carrier Card Schematic page 16; `kr260_carrier:1.0/board.xml`; `kr260_som:1.0/part0_pins.xml`.*
