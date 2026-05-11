################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/ARY_BOOL_SUM.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/RTDS_CTRL.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/VCO_FDBK.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/acpebb_states.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/control_blocks.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/control_functions.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/general_states.c \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/plecs_interface.c 

OBJS += \
./MVDC.CLI/ARY_BOOL_SUM.o \
./MVDC.CLI/RTDS_CTRL.o \
./MVDC.CLI/VCO_FDBK.o \
./MVDC.CLI/acpebb_states.o \
./MVDC.CLI/control_blocks.o \
./MVDC.CLI/control_functions.o \
./MVDC.CLI/general_states.o \
./MVDC.CLI/plecs_interface.o 

C_DEPS += \
./MVDC.CLI/ARY_BOOL_SUM.d \
./MVDC.CLI/RTDS_CTRL.d \
./MVDC.CLI/VCO_FDBK.d \
./MVDC.CLI/acpebb_states.d \
./MVDC.CLI/control_blocks.d \
./MVDC.CLI/control_functions.d \
./MVDC.CLI/general_states.d \
./MVDC.CLI/plecs_interface.d 


# Each subdirectory must supply rules for building sources it contributes
MVDC.CLI/ARY_BOOL_SUM.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/ARY_BOOL_SUM.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

MVDC.CLI/RTDS_CTRL.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/RTDS_CTRL.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

MVDC.CLI/VCO_FDBK.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/VCO_FDBK.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

MVDC.CLI/acpebb_states.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/acpebb_states.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

MVDC.CLI/control_blocks.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/control_blocks.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

MVDC.CLI/control_functions.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/control_functions.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

MVDC.CLI/general_states.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/general_states.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

MVDC.CLI/plecs_interface.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/plecs_interface.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


