################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MVDC.CLI/ARY_BOOL_SUM.c \
../src/MVDC.CLI/ENC_PROC.c \
../src/MVDC.CLI/RTDS_CTRL.c \
../src/MVDC.CLI/SIM_LR.c \
../src/MVDC.CLI/VCO_FDBK.c \
../src/MVDC.CLI/VF_CTRL.c \
../src/MVDC.CLI/acpebb_states.c \
../src/MVDC.CLI/boardtest_states.c \
../src/MVDC.CLI/control_blocks.c \
../src/MVDC.CLI/control_functions.c \
../src/MVDC.CLI/dfig_states.c \
../src/MVDC.CLI/general_states.c \
../src/MVDC.CLI/hesm_states.c \
../src/MVDC.CLI/plecs_interface.c \
../src/MVDC.CLI/twosie_states.c \
../src/MVDC.CLI/wind_states.c 

OBJS += \
./src/MVDC.CLI/ARY_BOOL_SUM.o \
./src/MVDC.CLI/ENC_PROC.o \
./src/MVDC.CLI/RTDS_CTRL.o \
./src/MVDC.CLI/SIM_LR.o \
./src/MVDC.CLI/VCO_FDBK.o \
./src/MVDC.CLI/VF_CTRL.o \
./src/MVDC.CLI/acpebb_states.o \
./src/MVDC.CLI/boardtest_states.o \
./src/MVDC.CLI/control_blocks.o \
./src/MVDC.CLI/control_functions.o \
./src/MVDC.CLI/dfig_states.o \
./src/MVDC.CLI/general_states.o \
./src/MVDC.CLI/hesm_states.o \
./src/MVDC.CLI/plecs_interface.o \
./src/MVDC.CLI/twosie_states.o \
./src/MVDC.CLI/wind_states.o 

C_DEPS += \
./src/MVDC.CLI/ARY_BOOL_SUM.d \
./src/MVDC.CLI/ENC_PROC.d \
./src/MVDC.CLI/RTDS_CTRL.d \
./src/MVDC.CLI/SIM_LR.d \
./src/MVDC.CLI/VCO_FDBK.d \
./src/MVDC.CLI/VF_CTRL.d \
./src/MVDC.CLI/acpebb_states.d \
./src/MVDC.CLI/boardtest_states.d \
./src/MVDC.CLI/control_blocks.d \
./src/MVDC.CLI/control_functions.d \
./src/MVDC.CLI/dfig_states.d \
./src/MVDC.CLI/general_states.d \
./src/MVDC.CLI/hesm_states.d \
./src/MVDC.CLI/plecs_interface.d \
./src/MVDC.CLI/twosie_states.d \
./src/MVDC.CLI/wind_states.d 


# Each subdirectory must supply rules for building sources it contributes
src/MVDC.CLI/%.o: ../src/MVDC.CLI/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


