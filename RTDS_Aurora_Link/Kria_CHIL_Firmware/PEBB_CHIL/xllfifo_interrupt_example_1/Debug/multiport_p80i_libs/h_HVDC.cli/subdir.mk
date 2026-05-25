################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/AC_CTRL.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/DC_CTRL.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/DC_DCPWM.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/THYPULSE.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/acControl.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/acControl_0.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/dcControl.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/dcControl_0.c 

OBJS += \
./multiport_p80i_libs/h_HVDC.cli/AC_CTRL.o \
./multiport_p80i_libs/h_HVDC.cli/DC_CTRL.o \
./multiport_p80i_libs/h_HVDC.cli/DC_DCPWM.o \
./multiport_p80i_libs/h_HVDC.cli/THYPULSE.o \
./multiport_p80i_libs/h_HVDC.cli/acControl.o \
./multiport_p80i_libs/h_HVDC.cli/acControl_0.o \
./multiport_p80i_libs/h_HVDC.cli/dcControl.o \
./multiport_p80i_libs/h_HVDC.cli/dcControl_0.o 

C_DEPS += \
./multiport_p80i_libs/h_HVDC.cli/AC_CTRL.d \
./multiport_p80i_libs/h_HVDC.cli/DC_CTRL.d \
./multiport_p80i_libs/h_HVDC.cli/DC_DCPWM.d \
./multiport_p80i_libs/h_HVDC.cli/THYPULSE.d \
./multiport_p80i_libs/h_HVDC.cli/acControl.d \
./multiport_p80i_libs/h_HVDC.cli/acControl_0.d \
./multiport_p80i_libs/h_HVDC.cli/dcControl.d \
./multiport_p80i_libs/h_HVDC.cli/dcControl_0.d 


# Each subdirectory must supply rules for building sources it contributes
multiport_p80i_libs/h_HVDC.cli/AC_CTRL.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/AC_CTRL.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/h_HVDC.cli/DC_CTRL.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/DC_CTRL.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/h_HVDC.cli/DC_DCPWM.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/DC_DCPWM.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/h_HVDC.cli/THYPULSE.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/THYPULSE.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/h_HVDC.cli/acControl.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/acControl.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/h_HVDC.cli/acControl_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/acControl_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/h_HVDC.cli/dcControl.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/dcControl.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

multiport_p80i_libs/h_HVDC.cli/dcControl_0.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/h_HVDC.cli/dcControl_0.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


