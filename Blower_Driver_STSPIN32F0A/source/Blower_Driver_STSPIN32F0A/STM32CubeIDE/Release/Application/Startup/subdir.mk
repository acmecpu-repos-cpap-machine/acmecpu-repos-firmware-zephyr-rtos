################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Application/Startup/startup_stm32f031c6tx.s 

OBJS += \
./Application/Startup/startup_stm32f031c6tx.o 

S_DEPS += \
./Application/Startup/startup_stm32f031c6tx.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Startup/startup_stm32f031c6tx.o: ../Application/Startup/startup_stm32f031c6tx.s
	arm-none-eabi-gcc -mcpu=cortex-m0 -c -x assembler-with-cpp -MMD -MP -MF"Application/Startup/startup_stm32f031c6tx.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@" "$<"

