################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/dac_rctimer_ui.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/main.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_api.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_config.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_interface.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_math.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_parameters.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_tasks.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/motor_control_protocol.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/motorcontrol.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/regular_conversion_manager.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/stm32f0xx_hal_msp.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/stm32f0xx_it.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/stm32f0xx_mc_it.c \
../Application/User/syscalls.c \
../Application/User/sysmem.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/ui_task.c \
C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/user_interface.c 

OBJS += \
./Application/User/dac_rctimer_ui.o \
./Application/User/main.o \
./Application/User/mc_api.o \
./Application/User/mc_config.o \
./Application/User/mc_interface.o \
./Application/User/mc_math.o \
./Application/User/mc_parameters.o \
./Application/User/mc_tasks.o \
./Application/User/motor_control_protocol.o \
./Application/User/motorcontrol.o \
./Application/User/regular_conversion_manager.o \
./Application/User/stm32f0xx_hal_msp.o \
./Application/User/stm32f0xx_it.o \
./Application/User/stm32f0xx_mc_it.o \
./Application/User/syscalls.o \
./Application/User/sysmem.o \
./Application/User/ui_task.o \
./Application/User/user_interface.o 

C_DEPS += \
./Application/User/dac_rctimer_ui.d \
./Application/User/main.d \
./Application/User/mc_api.d \
./Application/User/mc_config.d \
./Application/User/mc_interface.d \
./Application/User/mc_math.d \
./Application/User/mc_parameters.d \
./Application/User/mc_tasks.d \
./Application/User/motor_control_protocol.d \
./Application/User/motorcontrol.d \
./Application/User/regular_conversion_manager.d \
./Application/User/stm32f0xx_hal_msp.d \
./Application/User/stm32f0xx_it.d \
./Application/User/stm32f0xx_mc_it.d \
./Application/User/syscalls.d \
./Application/User/sysmem.d \
./Application/User/ui_task.d \
./Application/User/user_interface.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/dac_rctimer_ui.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/dac_rctimer_ui.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/dac_rctimer_ui.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/main.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/main.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/main.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/mc_api.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_api.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/mc_api.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/mc_config.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_config.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/mc_config.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/mc_interface.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_interface.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/mc_interface.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/mc_math.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_math.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/mc_math.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/mc_parameters.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_parameters.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/mc_parameters.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/mc_tasks.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/mc_tasks.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/mc_tasks.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/motor_control_protocol.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/motor_control_protocol.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/motor_control_protocol.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/motorcontrol.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/motorcontrol.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/motorcontrol.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/regular_conversion_manager.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/regular_conversion_manager.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/regular_conversion_manager.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/stm32f0xx_hal_msp.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/stm32f0xx_hal_msp.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/stm32f0xx_hal_msp.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/stm32f0xx_it.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/stm32f0xx_it.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/stm32f0xx_it.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/stm32f0xx_mc_it.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/stm32f0xx_mc_it.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/stm32f0xx_mc_it.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/syscalls.o: ../Application/User/syscalls.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/syscalls.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/sysmem.o: ../Application/User/sysmem.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/sysmem.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/ui_task.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/ui_task.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/ui_task.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/user_interface.o: C:/acmecpu/stm32_workspace/Blower_Driver_STSPIN32F0A/source/Blower_Driver_STSPIN32F0A/Src/user_interface.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DARM_MATH_CM0 -DUSE_HAL_DRIVER -DSTM32F031x6 -c -I../../Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc -I../../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/MCLib/F0xx/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/UILibrary/Inc -I../../MCSDK_v5.4.8/MotorControl/MCSDK/SystemDriveParams -I../../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Application/User/user_interface.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

