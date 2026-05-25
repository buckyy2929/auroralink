# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: Q:\kria\top\xllfifo_interrupt_example_1_system\_ide\scripts\systemdebugger_xllfifo_interrupt_example_1_system_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source Q:\kria\top\xllfifo_interrupt_example_1_system\_ide\scripts\systemdebugger_xllfifo_interrupt_example_1_system_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
source C:/Xilinx/Vitis/2022.1/scripts/vitis/util/zynqmp_utils.tcl
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/projects/Chil/kria/top/top/export/top/hw/top.xsa -mem-ranges [list {0x80000000 0xbfffffff} {0x400000000 0x5ffffffff} {0x1000000000 0x7fffffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
set mode [expr [mrd -value 0xFF5E0200] & 0xf]
targets -set -nocase -filter {name =~ "*A53*#0"}
rst -processor
dow C:/projects/Chil/kria/top/top/export/top/sw/top/boot/fsbl.elf
set bp_47_49_fsbl_bp [bpadd -addr &XFsbl_Exit]
con -block -timeout 60
bpremove $bp_47_49_fsbl_bp
targets -set -nocase -filter {name =~ "*A53*#0"}
rst -processor
dow Q:/kria/top/xllfifo_interrupt_example_1/Debug/xllfifo_interrupt_example_1.elf
configparams force-mem-access 0
bpadd -addr &main
