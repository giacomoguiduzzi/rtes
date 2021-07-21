################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/ST/workspace/WiFi_HTTP_Server/Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery.c \
D:/ST/workspace/WiFi_HTTP_Server/Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery_lcd.c 

OBJS += \
./Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery.o \
./Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery_lcd.o 

C_DEPS += \
./Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery.d \
./Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery_lcd.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery.o: D:/ST/workspace/WiFi_HTTP_Server/Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery.c Drivers/BSP/STM32F413H-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32F413xx -DUSE_STM32F413H_DISCO -c -I../../../Inc -I../../../Common/Inc -I../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../Drivers/BSP/STM32F413H-Discovery -I../../../Drivers/CMSIS/Include -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery_lcd.o: D:/ST/workspace/WiFi_HTTP_Server/Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery_lcd.c Drivers/BSP/STM32F413H-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32F413xx -DUSE_STM32F413H_DISCO -c -I../../../Inc -I../../../Common/Inc -I../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../Drivers/BSP/STM32F413H-Discovery -I../../../Drivers/CMSIS/Include -Os -ffunction-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/BSP/STM32F413H-Discovery/stm32f413h_discovery_lcd.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

