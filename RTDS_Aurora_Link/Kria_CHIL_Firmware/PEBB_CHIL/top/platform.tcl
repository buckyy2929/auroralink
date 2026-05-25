# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\projects\Chil\v1\top\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\projects\Chil\v1\top\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {top}\
-hw {Q:\kria\top.xsa}\
-arch {64-bit} -fsbl-target {psu_cortexa53_0} -out {C:/projects/Chil/v1}

platform write
domain create -name {standalone_psu_cortexa53_0} -display-name {standalone_psu_cortexa53_0} -os {standalone} -proc {psu_cortexa53_0} -runtime {cpp} -arch {64-bit} -support-app {empty_application}
platform generate -domains 
platform active {top}
domain active {zynqmp_fsbl}
domain active {zynqmp_pmufw}
domain active {standalone_psu_cortexa53_0}
platform generate -quick
bsp reload
platform generate
platform active {top}
platform generate -domains 
platform active {top}
domain active {zynqmp_fsbl}
domain active {standalone_psu_cortexa53_0}
domain active {zynqmp_pmufw}
bsp reload
bsp reload
bsp reload
platform generate -domains 
platform generate
platform active {top}
domain active {zynqmp_fsbl}
bsp reload
platform generate -domains 
platform generate
platform generate
platform generate
platform active {top}
domain active {standalone_psu_cortexa53_0}
domain active {zynqmp_fsbl}
domain active {zynqmp_pmufw}
bsp reload
bsp reload
bsp reload
domain active {standalone_psu_cortexa53_0}
bsp reload
bsp reload
domain active {zynqmp_fsbl}
bsp reload
domain active {zynqmp_pmufw}
bsp reload
bsp reload
domain active {standalone_psu_cortexa53_0}
bsp config stdin "psu_coresight_0"
bsp reload
bsp config stdin "psu_coresight_0"
bsp config stdin "psu_coresight_0"
bsp write
domain active {zynqmp_pmufw}
bsp reload
domain active {standalone_psu_cortexa53_0}
bsp config stdin "psu_coresight_0"
bsp config stdin "psu_coresight_0"
bsp config stdout "psu_coresight_0"
bsp write
platform active {top}
platform config -updatehw {C:/projects/Chil/kria/top.xsa}
domain active {zynqmp_fsbl}
bsp reload
bsp reload
domain active {standalone_psu_cortexa53_0}
bsp config stdin "psu_uart_1"
bsp config stdout "psu_uart_1"
bsp write
bsp reload
catch {bsp regenerate}
domain active {zynqmp_fsbl}
bsp config stdin "psu_uart_1"
bsp config stdout "psu_uart_1"
bsp write
bsp reload
catch {bsp regenerate}
domain active {zynqmp_pmufw}
bsp reload
bsp config stdin "none"
bsp write
platform generate -domains standalone_psu_cortexa53_0,zynqmp_fsbl 
domain active {standalone_psu_cortexa53_0}
bsp reload
bsp config stdout "psu_uart_1"
bsp reload
platform active {top}
platform generate -domains 
platform active {top}
platform config -updatehw {C:/projects/Chil/kria_drop_instr/kria/top.xsa}
platform generate -domains 
domain active {zynqmp_fsbl}
domain active {standalone_psu_cortexa53_0}
domain active {zynqmp_pmufw}
bsp reload
bsp reload
bsp reload
platform generate -domains 
platform active {top}
platform config -updatehw {C:/projects/Chil/kria/top.xsa}
platform generate -domains 
platform generate
platform generate
platform generate
platform active {top}
platform generate -domains 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform active {top}
domain active {standalone_psu_cortexa53_0}
bsp reload
platform generate -domains 
platform generate
platform generate
platform generate
platform generate
platform generate -domains standalone_psu_cortexa53_0 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform clean
platform generate
platform clean
platform generate
platform generate -domains standalone_psu_cortexa53_0,zynqmp_fsbl,zynqmp_pmufw 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains standalone_psu_cortexa53_0,zynqmp_fsbl,zynqmp_pmufw 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains standalone_psu_cortexa53_0 
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate
platform active {top}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform config -updatehw {C:/projects/Chil/MFESM_Chil/top.xsa}
platform generate -domains 
platform generate -domains 
platform generate
platform generate
platform generate
platform clean
platform generate
platform clean
platform generate
platform generate
