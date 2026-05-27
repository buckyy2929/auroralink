# ----------------------------------------------------------------------------
# build_aurora_loopback.tcl
#
# Standalone Aurora 8B/10B loopback bitstream for KR260, matching the
# IP parameter values in docs/milestone2/M2-A_auroralink_findings.md (sourced from the
# supplied top.xsa/design_1.hwh hardware handoff).
#
# ============================================================================
# !! READ THIS BEFORE RUNNING !!  This script is a STARTING SKELETON, not a
# turnkey build. It will require manual fixes at first elaboration. The known
# deviations from the production design (re-verified 2026-05-27 against
# design_1.hwh + PG046 + the kr260_carrier:1.0 board file + XTP743) are
# listed in Gap 10.7 / 10.8 of the M2-A findings doc; the short version:
#
#   1. (CRITICAL — Gap 10.8) STOCK KR260 REFCLK FREQUENCY MISMATCH.
#      The supplied design configures Aurora for 125 MHz refclk on
#      SOM240_2 pins C3/C4 (= package pins Y6/Y5). XTP743 page 16 shows
#      that the STOCK KR260 carrier card drives those same SOM240_2 C3/C4
#      pins with a 156.25 MHz LVDS oscillator (the standard 10G Ethernet
#      SFP+ reference). The supplied bitstream is therefore consistent
#      with a CUSTOM CHIL carrier card (not the stock starter kit) or
#      with a hardware-modified stock board. DO NOT RUN THIS BITSTREAM
#      AGAINST AN UNMODIFIED STOCK KR260 — confirm carrier hardware
#      first. (xsa.xml targets `xilinx.com:kr260_som:1.0`, the SOM-level
#      board file, which is consistent with either path.)
#
#   2. (Gap 10.7(b)) KR260 board file has NO `GT_125_REFCLK` interface.
#      `kr260_carrier:1.0/board.xml` defines only `sfp_led` (GPIO) and
#      `sfp_iic` (I2C). The `apply_bd_automation -rule
#      xilinx.com:bd_rule:board` call further down in this script
#      WILL FAIL on the refclk interface. The correct pattern (which
#      our aurora_loopback.xdc already uses) is manual external port
#      creation + explicit `set_property PACKAGE_PIN Y6/Y5`.
#      THIS CALL MUST BE REMOVED before the script will run.
#
#   3. (Gap 10.7(c)) The Aurora `loopback[2:0]` control pin is a
#      standard PG046 port (per PG046 §"Port Descriptions"), but is
#      NOT in the top-level port list of THIS particular IP
#      customisation — it is tied internally to 3'b000 (normal
#      operation). To drive PMA near-end loopback (3'b010) the IP
#      must be re-customised in the Vivado IP catalog to expose
#      `loopback`. The xlconstant + connect_bd_net for `loopback`
#      below assumes that re-customisation has been done.
#
#   4. (Gap 10.7(d)) PS interrupt vector pl_ps_irq0 must be configured
#      1-bit wide (PSU__USE__IRQ0 = 1 plus the separate Vivado
#      property C_NUM_F2P_0_INTR_INPUTS = 1). Without both, the
#      DT-visible IRQ number shifts and the UIO driver in §11.4
#      of the M2-A doc will not wake up.
#
#   5. (Gap 10.7(e)) This is a MINIMAL LOOPBACK SUBSET. Production
#      hwh also instantiates au_drop_0, au_modify_0, aurora_reset_0,
#      axi_bram_ctrl_0 + blk_mem_gen_0 scratch, three ILAs (ila_0,
#      ila_1, system_ila_0), three proc_sys_reset blocks, three
#      xpm_cdc_gen blocks, and an scl_adc_0 module. None of these
#      are replicated. Acceptable for first-light bring-up; NOT
#      equivalent to production.
#
#   6. Production design's clk_wiz_0 takes its primary input from an
#      external 25 MHz oscillator pin (C_PRIM_IN_FREQ = 25, verified
#      against XTP743 — the carrier has a 25 MHz LVCMOS fanout buffer
#      on net OSC_25M_CLK). This script derives Aurora's 50 MHz
#      init_clk from pl_clk0 (100 MHz) via a fresh clk_wiz, which is
#      a different clocking topology. Functionally equivalent for
#      Aurora init, but not bit-identical to production.
#
# CORRECTION FROM EARLIER REVISION (2026-05-27): an earlier comment in
# this header asserted that USER_DATA_S_AXIS_TX is not a valid bus
# interface and that TX pins must be wired signal-level. That was
# wrong. PG046 §"Port Descriptions" explicitly states:
#     "USER_DATA_S_AXIS_TX is the interface and the s_axi_tx_* ports
#      are grouped into that interface."
# So `connect_bd_intf_net … USER_DATA_S_AXIS_TX` calls below ARE
# correct. The earlier signal-level workaround is no longer needed.
#
# THE BEST PATH FORWARD: if the original Vivado project (top.bd /
# top.xpr) that generated top.xsa is available anywhere, prefer
# `write_bd_tcl` of THAT to this hand-written skeleton. It will be
# more accurate than anything reconstructed from the hwh.
# ============================================================================
#
# Sourced parameters (every value below was re-read from design_1.hwh on
# 2026-05-27 — `unzip -p top.xsa design_1.hwh` then grep):
#   aurora_8b10b_0 VLNV     = xilinx.com:ip:aurora_8b10b:11.1
#   C_AURORA_LANES          = 1
#   C_LANE_WIDTH            = 4   (-> 32-bit AXI-Stream)
#   C_LINE_RATE             = 2   (Gbps)
#   C_REFCLK_FREQUENCY      = 125 (MHz)
#   C_INIT_CLK              = 50.0 (MHz)
#   DRP_FREQ                = 50.0000
#   C_DRP_IF                = false  (DRP wrapped internally)
#   Interface_Mode          = Framing
#   Dataflow_Config         = Duplex
#   Flow_Mode               = None
#   Backchannel_mode        = Sidebands
#   CHANNEL_ENABLE          = X0Y4
#   C_START_QUAD            = Quad_X0Y1
#   C_REFCLK_LOC_P / N      = Y6 / Y5
#   C_REFCLK_SOURCE         = "X0Y4 clk0"
#   C_GT_CLOCK_1            = GTHQ0
#   axi_fifo_mm_s_0 VLNV    = xilinx.com:ip:axi_fifo_mm_s:4.2
#   TX/RX FIFO depth        = 512 / 512
#   FIFO MMIO range         = 0xA0010000 - 0xA001FFFF (via M_AXI_HPM0_FPD)
#   FIFO IRQ                = axi_fifo_mm_s_0.interrupt -> pl_ps_irq0[0:0]
#   PSU__USE__IRQ0          = 1
#   C_NUM_F2P_0_INTR_INPUTS = 1
#   Board                   = xilinx.com:kr260_som:1.0  (NOT :part0:1.1)
#   Part                    = xck26-sfvc784-2LV-c
# ----------------------------------------------------------------------------

# --- Project knobs ----------------------------------------------------------
set proj_name      "aurora_loopback"
set proj_dir       [file normalize "out/vivado/${proj_name}_proj"]
set out_dir        [file normalize "out/vivado"]
set part           "xck26-sfvc784-2LV-c"
set board_part     "xilinx.com:kr260_som:1.0"
set top_module     "design_1_wrapper"
set bd_name        "design_1"
set jobs           4

# Repository root (one level up from scripts/vivado/)
set repo_root      [file normalize [file join [file dirname [info script]] ".." ".."]]
puts "INFO: repo_root = $repo_root"

# --- Aurora line-rate / refclk knobs (Gap 10.8 resolution) ------------------
#
# Two supported configurations:
#
#   PRODUCTION TARGET  : C_REFCLK_FREQUENCY = 125 MHz, C_LINE_RATE = 2 Gbps
#                        Matches design_1.hwh exactly. Requires a custom
#                        CHIL carrier card (or hardware-modified stock
#                        KR260) that delivers 125 MHz on SOM240_2 C3/C4.
#
#   STOCK-KR260 LAB    : C_REFCLK_FREQUENCY = 156.25 MHz, C_LINE_RATE = 2.5 Gbps
#   VARIANT (default)    Matches what the stock KR260 carrier physically
#                        delivers (XTP743 page 16). Same integer GTH PLL
#                        multiplier as production (refclk * 16). Software,
#                        FIFO, IRQ, framing, TLAST behaviour identical;
#                        only Aurora user_clk_out shifts from 50 MHz to
#                        62.5 MHz. Used for S3 verification on stock
#                        hardware without an oscillator swap.
#
# Pass via -tclargs on the command line:
#   vivado -mode batch -source build_aurora_loopback.tcl \
#          -tclargs --refclk_mhz 156.25 --line_rate_gbps 2.5
#
# Defaults below are STOCK-KR260 LAB VARIANT because that is what runs on
# the hardware actually on the desk today. Flip to 125/2 once the
# production carrier card is in place.
set aurora_refclk_mhz     156.25
set aurora_line_rate_gbps 2.5

# Simple argv parser (Vivado passes -tclargs after the script name)
if { [info exists argv] } {
    for { set i 0 } { $i < [llength $argv] } { incr i } {
        switch -- [lindex $argv $i] {
            "--refclk_mhz"      { incr i; set aurora_refclk_mhz     [lindex $argv $i] }
            "--line_rate_gbps"  { incr i; set aurora_line_rate_gbps [lindex $argv $i] }
        }
    }
}

# Sanity-check the GTH PLL math: line_rate (Gbps) * 1000 / refclk (MHz) must
# be a small integer (typically 8 / 16 / 32 / 40 / 64) for a clean PLL lock.
set _ratio [expr {$aurora_line_rate_gbps * 1000.0 / $aurora_refclk_mhz}]
set _ratio_int [expr {int(round($_ratio))}]
if { abs($_ratio - $_ratio_int) > 0.001 } {
    puts "ERROR: refclk_mhz=$aurora_refclk_mhz, line_rate_gbps=$aurora_line_rate_gbps"
    puts "       gives PLL multiplier $_ratio -- not an integer."
    puts "       GTH PLL cannot lock cleanly. Pick a refclk/line_rate that divides cleanly."
    puts "       Common combos: 125/2, 125/2.5, 125/5, 156.25/2.5, 156.25/5, 156.25/10."
    exit 4
}
set aurora_user_clk_mhz [expr {$aurora_line_rate_gbps * 1000.0 * 8.0 / 10.0 / 32.0}]
puts "INFO: Aurora configuration:"
puts "INFO:   refclk         = $aurora_refclk_mhz MHz"
puts "INFO:   line rate      = $aurora_line_rate_gbps Gbps"
puts "INFO:   PLL multiplier = ${_ratio_int}x"
puts "INFO:   user_clk_out   = $aurora_user_clk_mhz MHz (derived)"

# --- Sanity: Vivado version + board file availability -----------------------
if { [catch { version }] } {
    puts "ERROR: this script must be sourced from within Vivado."
    exit 1
}
set vivado_v [version -short]
puts "INFO: Vivado [version -short]"
if { [package vcompare $vivado_v 2022.1] < 0 } {
    puts "WARNING: KR260 board files first shipped in Vivado 2022.1."
    puts "         You are on $vivado_v -- proceed only if you know what you are doing."
}

if { [llength [get_board_parts $board_part]] == 0 } {
    puts "ERROR: KR260 board part '$board_part' is not installed."
    puts "       In Vivado: Tools -> Vivado Store -> Boards, search 'kr260'."
    exit 2
}

# --- Make sure output directories exist -------------------------------------
file mkdir $proj_dir
file mkdir $out_dir

# --- Create project ---------------------------------------------------------
puts "INFO: creating project at $proj_dir"
create_project -force $proj_name $proj_dir -part $part
set_property board_part $board_part [current_project]

# --- Build block design -----------------------------------------------------
puts "INFO: creating block design $bd_name"
create_bd_design $bd_name

# Zynq UltraScale+ PS with KR260 board defaults
# NOTE: must also force PSU__USE__IRQ0 = 1 and the IRQ vector width to 1
# (C_NUM_F2P_0_INTR_INPUTS = 1) to match the production design --
# without this the DT-visible IRQ number shifts and UIO breaks.
create_bd_cell -type ip -vlnv xilinx.com:ip:zynq_ultra_ps_e:* zynq_ultra_ps_e_0
apply_bd_automation -rule xilinx.com:bd_rule:zynq_ultra_ps_e \
    -config { apply_board_preset "1" Master "Disable" Slave "Disable" } \
    [get_bd_cells zynq_ultra_ps_e_0]
set_property -dict [list \
    CONFIG.PSU__USE__IRQ0          {1} \
    CONFIG.PSU__USE__M_AXI_GP0     {1} \
] [get_bd_cells zynq_ultra_ps_e_0]

# Aurora 8b/10b -- parameters match design_1.hwh
create_bd_cell -type ip -vlnv xilinx.com:ip:aurora_8b10b:* aurora_8b10b_0
set_property -dict [list \
    CONFIG.C_AURORA_LANES     {1}                          \
    CONFIG.C_LANE_WIDTH       {4}                          \
    CONFIG.C_LINE_RATE        $aurora_line_rate_gbps       \
    CONFIG.C_REFCLK_FREQUENCY $aurora_refclk_mhz           \
    CONFIG.C_INIT_CLK         {50.0}                       \
    CONFIG.DRP_FREQ           {50.0}                       \
    CONFIG.Interface_Mode     {Framing}   \
    CONFIG.Dataflow_Config    {Duplex}    \
    CONFIG.Flow_Mode          {None}      \
    CONFIG.Backchannel_mode   {Sidebands} \
    CONFIG.CHANNEL_ENABLE     {X0Y4}      \
    CONFIG.C_START_QUAD       {Quad_X0Y1} \
    CONFIG.C_START_LANE       {X0Y4}      \
] [get_bd_cells aurora_8b10b_0]

# AXI4-Stream FIFO (PG080) -- 32-bit, interrupt enabled
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_fifo_mm_s:* axi_fifo_mm_s_0
set_property -dict [list \
    CONFIG.C_USE_TX_DATA      {1}   \
    CONFIG.C_USE_RX_DATA      {1}   \
    CONFIG.C_DATA_INTERFACE_TYPE {0} \
    CONFIG.C_AXIS_TDATA_WIDTH {32} \
] [get_bd_cells axi_fifo_mm_s_0]

# Near-end PMA loopback constant -- tie aurora_8b10b_0.loopback to 3'b010.
#
# Why PMA, not PCS: PMA loopback (3'b010) loops the data AFTER the serializer,
# exercising the GTH transceiver's full serdes path. PCS loopback (3'b001)
# loops BEFORE the serializer, leaving the serdes untested. The KR260
# explanation doc (explaination_docs/KR260_auroralink.md \u00a78.3) explicitly
# selects PMA; this TCL matches that recommendation.
#
# WARNING: this connect_bd_net WILL FAIL with the IP customisation that
# matches the production hwh, because that customisation does NOT expose
# `loopback` at the IP top-level. Before this block runs you must either:
#   (a) re-customise the Aurora IP to expose `loopback` as a top-level
#       port (Vivado IP wizard -> "Show external interfaces"), or
#   (b) wrap the IP with thin RTL that ties `loopback` to 3'b010 inside
#       the wrapper.
# Until then this section is documentation, not a working build step.
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:* loopback_const
set_property -dict [list \
    CONFIG.CONST_VAL          {2} \
    CONFIG.CONST_WIDTH        {3} \
] [get_bd_cells loopback_const]
if { [llength [get_bd_pins -quiet aurora_8b10b_0/loopback]] > 0 } {
    connect_bd_net [get_bd_pins loopback_const/dout] \
                   [get_bd_pins aurora_8b10b_0/loopback]
} else {
    puts "WARNING: aurora_8b10b_0/loopback pin not exposed by this IP customisation."
    puts "         See Gap 10.7 in docs/milestone2/M2-A_auroralink_findings.md."
    puts "         You must re-customise the Aurora IP or wrap it before bitstream gen."
}

# A clock wizard to derive 50 MHz init clock from pl_clk0 (100 MHz default)
create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:* clk_wiz_50
set_property -dict [list \
    CONFIG.PRIM_IN_FREQ       {100.000} \
    CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {50.000} \
    CONFIG.USE_RESET          {false} \
] [get_bd_cells clk_wiz_50]

connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] \
               [get_bd_pins clk_wiz_50/clk_in1]
connect_bd_net [get_bd_pins clk_wiz_50/clk_out1] \
               [get_bd_pins aurora_8b10b_0/init_clk]

# AXI interconnect from PS HPM0 -> FIFO control
apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config { Master "/zynq_ultra_ps_e_0/M_AXI_HPM0_FPD" Clk "Auto" } \
    [get_bd_intf_pins axi_fifo_mm_s_0/S_AXI]

# Connect Aurora user-side AXI-Stream to the FIFO.
#
# WARNING: the IP customisation that produced the production hwh wraps the
# TX side as signal-level ports (s_axi_tx_tvalid, s_axi_tx_tready,
# s_axi_tx_tlast, and the corresponding tdata bundled inside the bus
# interface aurora_8b10b_0_USER_DATA_M_AXI_RX -- note "AXI" not "AXIS").
# With that customisation, the two connect_bd_intf_net calls below will
# fail because USER_DATA_S_AXIS_TX is not a top-level interface.
#
# Two recovery paths:
#   (1) Re-customise the Aurora IP to expose USER_DATA_S_AXIS_TX as a
#       packaged AXI-Stream interface (PG046 supports both wrappings).
#   (2) Replace these two lines with per-signal connect_bd_net for
#       s_axi_tx_tdata/tvalid/tready/tlast and m_axi_rx_tdata/tvalid/tlast.
#
# See Gap 10.7 in docs/milestone2/M2-A_auroralink_findings.md and the §2.2 port table
# for the actual port names this customisation exposes.
if { [llength [get_bd_intf_pins -quiet aurora_8b10b_0/USER_DATA_S_AXIS_TX]] > 0 } {
    connect_bd_intf_net [get_bd_intf_pins aurora_8b10b_0/USER_DATA_S_AXIS_TX] \
                        [get_bd_intf_pins axi_fifo_mm_s_0/AXI_STR_TXD]
    connect_bd_intf_net [get_bd_intf_pins aurora_8b10b_0/USER_DATA_M_AXIS_RX] \
                        [get_bd_intf_pins axi_fifo_mm_s_0/AXI_STR_RXD]
} else {
    puts "WARNING: Aurora packaged AXI-Stream interfaces not exposed in this customisation."
    puts "         See Gap 10.7 and \u00a72.2 of docs/milestone2/M2-A_auroralink_findings.md."
    puts "         You must either re-customise the Aurora IP, or replace the two"
    puts "         connect_bd_intf_net calls above with per-signal connect_bd_net calls"
    puts "         for s_axi_tx_t{data,valid,ready,last} <-> AXI_STR_TXD and m_axi_rx_t{...}."
}

# Use the Aurora user_clk_out as the AXI-Stream clock for the FIFO data path
connect_bd_net [get_bd_pins aurora_8b10b_0/user_clk_out] \
               [get_bd_pins axi_fifo_mm_s_0/s_axis_aclk]
connect_bd_net [get_bd_pins aurora_8b10b_0/user_clk_out] \
               [get_bd_pins axi_fifo_mm_s_0/m_axis_aclk]

# FIFO interrupt -> PS pl_ps_irq0[0]
connect_bd_net [get_bd_pins axi_fifo_mm_s_0/interrupt] \
               [get_bd_pins zynq_ultra_ps_e_0/pl_ps_irq0]

# GT refclk: manual external port pair (pins Y6/Y5; frequency = $aurora_refclk_mhz MHz).
# Note (Gap 10.7(b) / verification V2): the kr260_carrier:1.0 board file
# defines NO `GT_125_REFCLK` board interface (it only defines sfp_led GPIO
# and sfp_iic I2C). The `apply_bd_automation -rule xilinx.com:bd_rule:board`
# call that an earlier draft used here would fail with
# "ERROR: [BD 41-1273] No Board_Interface with name GT_125_REFCLK exists".
# The correct pattern is to create external ports and let aurora_loopback.xdc
# pin them down with explicit PACKAGE_PIN Y6/Y5 + a parametric `create_clock`
# (the create_clock is emitted by THIS TCL below, NOT by the static XDC,
# because the period depends on $aurora_refclk_mhz).
#
# Gap 10.8 resolution: on stock KR260 the oscillator is 156.25 MHz, so
# defaults at the top of this file use 156.25 MHz / 2.5 Gbps. For a
# custom CHIL carrier card with 125 MHz oscillator + production target,
# invoke with -tclargs --refclk_mhz 125 --line_rate_gbps 2.
set _refclk_hz [expr {int($aurora_refclk_mhz * 1.0e6)}]
create_bd_port -dir I -from 0 -to 0 -type clk GT_125_REFCLK_clk_p
create_bd_port -dir I -from 0 -to 0 -type clk GT_125_REFCLK_clk_n
set_property CONFIG.FREQ_HZ $_refclk_hz [get_bd_ports GT_125_REFCLK_clk_p]
set_property CONFIG.FREQ_HZ $_refclk_hz [get_bd_ports GT_125_REFCLK_clk_n]
# Connect to the Aurora IP refclk pins. Names below assume the production
# customisation (signal-level `gt_refclk1_p` / `gt_refclk1_n` as confirmed
# by design_1.hwh §2.2). If your re-customised IP exposes them as a
# packaged GT_DIFF_REFCLK1 bus interface, replace these two connect_bd_net
# calls with a single `connect_bd_intf_net` against that interface.
if { [llength [get_bd_pins -quiet aurora_8b10b_0/gt_refclk1_p]] > 0 } {
    connect_bd_net [get_bd_ports GT_125_REFCLK_clk_p] \
                   [get_bd_pins  aurora_8b10b_0/gt_refclk1_p]
    connect_bd_net [get_bd_ports GT_125_REFCLK_clk_n] \
                   [get_bd_pins  aurora_8b10b_0/gt_refclk1_n]
} else {
    puts "WARNING: aurora_8b10b_0/gt_refclk1_p not found; refclk wiring left open."
    puts "         Inspect the IP's actual top-level ports and wire the refclk by hand."
}

# Validate, wrap, save
validate_bd_design
save_bd_design

make_wrapper -files [get_files ${bd_name}.bd] -top
add_files -norecurse ${proj_dir}/${proj_name}.gen/sources_1/bd/${bd_name}/hdl/${bd_name}_wrapper.v
set_property top $top_module [current_fileset]

# --- Constraints ------------------------------------------------------------
# XDC adds explicit refclk pin LOCATIONS (Y6/Y5) + asynchronous clock-group
# constraint. The `create_clock` period is INJECTED here because it depends
# on $aurora_refclk_mhz which is set at the top of this script (Gap 10.8
# resolution -- stock KR260 needs 156.25 MHz, production needs 125 MHz).
add_files -fileset constrs_1 -norecurse \
    [file join $repo_root "scripts/vivado/aurora_loopback.xdc"]

# Emit the parametric create_clock as an additional XDC file in-memory.
# Vivado allows multiple XDC sources; this one carries only the create_clock
# whose period derives from $aurora_refclk_mhz.
set _period_ns [format "%.3f" [expr {1000.0 / $aurora_refclk_mhz}]]
set _create_clk_xdc [file join $proj_dir "gen_refclk.xdc"]
set _fh [open $_create_clk_xdc "w"]
puts $_fh "# Auto-generated by build_aurora_loopback.tcl"
puts $_fh "# refclk = $aurora_refclk_mhz MHz, period = $_period_ns ns"
puts $_fh "create_clock -period $_period_ns -name gt_refclk \[get_ports GT_125_REFCLK_clk_p\]"
close $_fh
add_files -fileset constrs_1 -norecurse $_create_clk_xdc
puts "INFO: emitted parametric create_clock at $_create_clk_xdc"

# --- Synthesis, implementation, bitstream ----------------------------------
update_compile_order -fileset sources_1
launch_runs synth_1 -jobs $jobs
wait_on_run synth_1
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: synthesis failed -- see logs in ${proj_dir}"
    exit 3
}

launch_runs impl_1 -to_step write_bitstream -jobs $jobs
wait_on_run impl_1
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: implementation/bitstream failed -- see logs in ${proj_dir}"
    exit 4
}

# --- Export artifacts -------------------------------------------------------
set bit_src [file join ${proj_dir} ${proj_name}.runs impl_1 ${top_module}.bit]
set bit_dst [file join $out_dir design_1.bit]
if {[file exists $bit_src]} {
    file copy -force $bit_src $bit_dst
    puts "INFO: bitstream -> $bit_dst"
} else {
    puts "ERROR: expected bitstream at $bit_src but it is not there"
    exit 5
}

# Export XSA (hardware handoff) for downstream Vitis/PetaLinux device-tree
# overlay generation (out/vivado/design_1.xsa -> dtbo via xsct/xilinx-dt).
write_hw_platform -fixed -force -file [file join $out_dir design_1.xsa]
puts "INFO: hardware handoff XSA -> [file join $out_dir design_1.xsa]"

puts "OK: aurora_loopback build complete."
puts "    Next: see docs/milestone2/M2-A_auroralink_findings.md \u00a711 for the on-board verification checklist."
