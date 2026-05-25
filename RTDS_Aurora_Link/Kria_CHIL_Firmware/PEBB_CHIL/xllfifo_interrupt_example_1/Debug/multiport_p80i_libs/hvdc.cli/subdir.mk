################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Copy\ of\ TestVectors_with_modes.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/HW_IF.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC_.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Modul_test.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Modul_test_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/PTR_TEST.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/PTR_TEST2.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Sub.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Sub_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/TestVectors.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/TestVectorsFunc.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/TestVectors_with_modes.c 

OBJS += \
./multiport_p80i_libs/hvdc.cli/Copy\ of\ TestVectors_with_modes.o \
./multiport_p80i_libs/hvdc.cli/HW_IF.o \
./multiport_p80i_libs/hvdc.cli/MEMC.o \
./multiport_p80i_libs/hvdc.cli/MEMC_.o \
./multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt.o \
./multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt_0.o \
./multiport_p80i_libs/hvdc.cli/Modul_test.o \
./multiport_p80i_libs/hvdc.cli/Modul_test_0.o \
./multiport_p80i_libs/hvdc.cli/PTR_TEST.o \
./multiport_p80i_libs/hvdc.cli/PTR_TEST2.o \
./multiport_p80i_libs/hvdc.cli/Sub.o \
./multiport_p80i_libs/hvdc.cli/Sub_0.o \
./multiport_p80i_libs/hvdc.cli/TestVectors.o \
./multiport_p80i_libs/hvdc.cli/TestVectorsFunc.o \
./multiport_p80i_libs/hvdc.cli/TestVectors_with_modes.o 

C_DEPS += \
./multiport_p80i_libs/hvdc.cli/Copy\ of\ TestVectors_with_modes.d \
./multiport_p80i_libs/hvdc.cli/HW_IF.d \
./multiport_p80i_libs/hvdc.cli/MEMC.d \
./multiport_p80i_libs/hvdc.cli/MEMC_.d \
./multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt.d \
./multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt_0.d \
./multiport_p80i_libs/hvdc.cli/Modul_test.d \
./multiport_p80i_libs/hvdc.cli/Modul_test_0.d \
./multiport_p80i_libs/hvdc.cli/PTR_TEST.d \
./multiport_p80i_libs/hvdc.cli/PTR_TEST2.d \
./multiport_p80i_libs/hvdc.cli/Sub.d \
./multiport_p80i_libs/hvdc.cli/Sub_0.d \
./multiport_p80i_libs/hvdc.cli/TestVectors.d \
./multiport_p80i_libs/hvdc.cli/TestVectorsFunc.d \
./multiport_p80i_libs/hvdc.cli/TestVectors_with_modes.d 


# Each subdirectory must supply rules for building sources it contributes
multiport_p80i_libs/hvdc.cli/Copy\ of\ TestVectors_with_modes.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Copy\ of\ TestVectors_with_modes.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"multiport_p80i_libs/hvdc.cli/Copy of TestVectors_with_modes.d" -MT"multiport_p80i_libs/hvdc.cli/Copy\ of\ TestVectors_with_modes.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/HW_IF.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/HW_IF.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/MEMC.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/MEMC_.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC_.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/MEMC_test_leg_PDO_pkt_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/Modul_test.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Modul_test.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/Modul_test_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Modul_test_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/PTR_TEST.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/PTR_TEST.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/PTR_TEST2.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/PTR_TEST2.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/Sub.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Sub.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/Sub_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/Sub_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/TestVectors.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/TestVectors.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/TestVectorsFunc.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/TestVectorsFunc.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli/TestVectors_with_modes.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli/TestVectors_with_modes.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


