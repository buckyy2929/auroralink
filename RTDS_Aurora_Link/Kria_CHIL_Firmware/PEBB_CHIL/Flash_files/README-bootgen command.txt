1. Copy in the updated files (fsbl.elf, pmufw.elf, top.bit, xllfifo_interrupt_example_1.elf)
cd C:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/Flash_files
In Xilinx XSCT console:
bootgen -w -arch zynqmp -image PEBB_KRIA_CHIL.bif -o boot.bin