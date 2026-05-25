################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_1.c 

OBJS += \
./multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl.o \
./multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_0.o \
./multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_1.o 

C_DEPS += \
./multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl.d \
./multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_0.d \
./multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_1.d 


# Each subdirectory must supply rules for building sources it contributes
multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_1.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/hvdc.cli_/src/FactoryTestCtrl_1.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


