################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/plecs_interface.c 

OBJS += \
./multiport_p80i_libs/MVDC.CLI/plecs_interface.o 

C_DEPS += \
./multiport_p80i_libs/MVDC.CLI/plecs_interface.d 


# Each subdirectory must supply rules for building sources it contributes
multiport_p80i_libs/MVDC.CLI/plecs_interface.o: C:/projects/Multiport/multiport_git/multiport_p80i_libs/MVDC.CLI/plecs_interface.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


