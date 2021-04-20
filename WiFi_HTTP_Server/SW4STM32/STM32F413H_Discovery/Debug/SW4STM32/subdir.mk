################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
D:/ST/workspace/WiFi_HTTP_Server/SW4STM32/startup_stm32f413xx.s 

OBJS += \
./SW4STM32/startup_stm32f413xx.o 

S_DEPS += \
./SW4STM32/startup_stm32f413xx.d 


# Each subdirectory must supply rules for building sources it contributes
SW4STM32/startup_stm32f413xx.o: D:/ST/workspace/WiFi_HTTP_Server/SW4STM32/startup_stm32f413xx.s SW4STM32/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -c -x assembler-with-cpp -MMD -MP -MF"SW4STM32/startup_stm32f413xx.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

