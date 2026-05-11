# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: C:\projects\Multiport\multiport_git\Kria_CHIL_Firmware\PEBB_CHIL\xllfifo_interrupt_example_1_system\_ide\scripts\systemdebugger_xllfifo_interrupt_example_1_system_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source C:\projects\Multiport\multiport_git\Kria_CHIL_Firmware\PEBB_CHIL\xllfifo_interrupt_example_1_system\_ide\scripts\systemdebugger_xllfifo_interrupt_example_1_system_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
source C:/Xilinx/Vitis/2022.1/scripts/vitis/util/zynqmp_utils.tcl
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Xilinx SCK-KR XFL1TWVO5ZP2A" && level==0 && jtag_device_ctx=="jsn-SCK-KR-XFL1TWVO5ZP2A-04724093-0"}
fpga -file C:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/xllfifo_interrupt_example_1/_ide/bitstream/top.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/hw/top.xsa -mem-ranges [list {0x80000000 0xbfffffff} {0x400000000 0x5ffffffff} {0x1000000000 0x7fffffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
set mode [expr [mrd -value 0xFF5E0200] & 0xf]
mask_write 0xFF5E0200 0xf000 0
targets -set -nocase -filter {name =~ "*A53*#0"}
rst -processor
dow C:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/boot/fsbl.elf
set bp_37_59_fsbl_bp [bpadd -addr &XFsbl_Exit]
con -block -timeout 60
bpremove $bp_37_59_fsbl_bp
targets -set -nocase -filter {name =~ "*A53*#0"}
rst -processor
dow C:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/xllfifo_interrupt_example_1/Debug/xllfifo_interrupt_example_1.elf
configparams force-mem-access 0
bpadd -addr &main
