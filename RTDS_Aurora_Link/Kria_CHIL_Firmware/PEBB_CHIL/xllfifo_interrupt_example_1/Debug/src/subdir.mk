################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/Axi_IO.c \
../src/PEBB_Control.c \
../src/PEBB_Control_0.c \
../src/PEBB_Control_1.c \
../src/PEBB_Control_2.c \
../src/PEBB_Control_3.c \
../src/UART.c \
../src/codegen.c \
../src/main.c 

OBJS += \
./src/Axi_IO.o \
./src/PEBB_Control.o \
./src/PEBB_Control_0.o \
./src/PEBB_Control_1.o \
./src/PEBB_Control_2.o \
./src/PEBB_Control_3.o \
./src/UART.o \
./src/codegen.o \
./src/main.o 

C_DEPS += \
./src/Axi_IO.d \
./src/PEBB_Control.d \
./src/PEBB_Control_0.d \
./src/PEBB_Control_1.d \
./src/PEBB_Control_2.d \
./src/PEBB_Control_3.d \
./src/UART.d \
./src/codegen.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 gcc compiler'
	aarch64-none-elf-gcc -Wall -O0 -g3 -IC:/projects/Multiport/multiport_git/Kria_CHIL_Firmware/PEBB_CHIL/top/export/top/sw/top/standalone_psu_cortexa53_0/bspinclude/include -IC:/projects/Multiport/multiport_git/multiport_p80i_libs -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


