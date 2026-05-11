################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FACTRY_TST.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_1.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_2.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_3.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/HW_IF.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/MEMC.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/PTR_TEST.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/PTR_TEST2.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/Sub.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/Sub_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/TestVectors_with_modes.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/pib_pwm.c 

OBJS += \
./multiport_p80i_libs/hvdc.cli_/FACTRY_TST.o \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl.o \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_0.o \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_1.o \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_2.o \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_3.o \
./multiport_p80i_libs/hvdc.cli_/HW_IF.o \
./multiport_p80i_libs/hvdc.cli_/MEMC.o \
./multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt.o \
./multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt_0.o \
./multiport_p80i_libs/hvdc.cli_/PTR_TEST.o \
./multiport_p80i_libs/hvdc.cli_/PTR_TEST2.o \
./multiport_p80i_libs/hvdc.cli_/Sub.o \
./multiport_p80i_libs/hvdc.cli_/Sub_0.o \
./multiport_p80i_libs/hvdc.cli_/TestVectors_with_modes.o \
./multiport_p80i_libs/hvdc.cli_/pib_pwm.o 

C_DEPS += \
./multiport_p80i_libs/hvdc.cli_/FACTRY_TST.d \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl.d \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_0.d \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_1.d \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_2.d \
./multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_3.d \
./multiport_p80i_libs/hvdc.cli_/HW_IF.d \
./multiport_p80i_libs/hvdc.cli_/MEMC.d \
./multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt.d \
./multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt_0.d \
./multiport_p80i_libs/hvdc.cli_/PTR_TEST.d \
./multiport_p80i_libs/hvdc.cli_/PTR_TEST2.d \
./multiport_p80i_libs/hvdc.cli_/Sub.d \
./multiport_p80i_libs/hvdc.cli_/Sub_0.d \
./multiport_p80i_libs/hvdc.cli_/TestVectors_with_modes.d \
./multiport_p80i_libs/hvdc.cli_/pib_pwm.d 


# Each subdirectory must supply rules for building sources it contributes
multiport_p80i_libs/hvdc.cli_/FACTRY_TST.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FACTRY_TST.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_1.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_1.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_2.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_2.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_3.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/FactoryTestCtrl_3.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/HW_IF.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/HW_IF.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/MEMC.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/MEMC.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/MEMC_test_leg_PDO_pkt_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/PTR_TEST.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/PTR_TEST.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/PTR_TEST2.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/PTR_TEST2.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/Sub.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/Sub.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/Sub_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/Sub_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/TestVectors_with_modes.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/TestVectors_with_modes.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/pib_pwm.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/pib_pwm.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


