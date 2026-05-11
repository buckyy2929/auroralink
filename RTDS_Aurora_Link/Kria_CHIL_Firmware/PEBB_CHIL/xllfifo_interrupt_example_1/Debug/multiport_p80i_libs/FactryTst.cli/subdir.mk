################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FACTRY_TST.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_1.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_2.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_3.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/pib_pwm.c 

OBJS += \
./multiport_p80i_libs/FactryTst.cli/FACTRY_TST.o \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl.o \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_0.o \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_1.o \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_2.o \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_3.o \
./multiport_p80i_libs/FactryTst.cli/pib_pwm.o 

C_DEPS += \
./multiport_p80i_libs/FactryTst.cli/FACTRY_TST.d \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl.d \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_0.d \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_1.d \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_2.d \
./multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_3.d \
./multiport_p80i_libs/FactryTst.cli/pib_pwm.d 


# Each subdirectory must supply rules for building sources it contributes
multiport_p80i_libs/FactryTst.cli/FACTRY_TST.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FACTRY_TST.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_1.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_1.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_2.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_2.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_3.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/FactoryTestCtrl_3.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/FactryTst.cli/pib_pwm.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/FactryTst.cli/pib_pwm.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


