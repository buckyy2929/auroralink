proc boot_jtag { } {
############################
# Switch to JTAG boot mode #
############################
targets -set -filter {name =~ "PSU"}
# update multiboot to ZERO
mwr 0xffca0010 0x0
# change boot mode to JTAG
mwr 0xff5e0200 0x0100
# reset
rst -system
}

connect
boot_jtag

after 2000
targets -set -filter {name =~ "PSU"}
#fpga "Q:/kria/top/top/export/top/hw/top.bit"
fpga "C:/projects/Chil/MFESM_Chil/top/hw/top.bit"
mwr 0xffca0038 0x1FF

# Download pmufw.elf
targets -set -filter {name =~ "MicroBlaze PMU"}
after 500
dow C:/projects/Chil/MFESM_Chil/top/export/top/sw/top/boot/pmufw.elf
con
after 500

# Select A53 Core 0
targets -set -filter {name =~ "Cortex-A53 #0"}
rst -processor -clear-registers
dow C:/projects/Chil/MFESM_Chil/top/export/top/sw/top/boot/fsbl.elf
con
after 10000
stop

#dow Q:/kria/top/xllfifo_polling_example_1/Debug/xllfifo_polling_example_1.elf
dow C:/projects/Chil/MFESM_Chil/xllfifo_interrupt_example_1/Debug/xllfifo_interrupt_example_1.elf
after 500
con