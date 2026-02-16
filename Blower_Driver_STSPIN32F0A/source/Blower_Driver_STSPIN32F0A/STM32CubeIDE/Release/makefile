################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/MotorControl/subdir.mk
-include Drivers/STM32F0xx_HAL_Driver/subdir.mk
-include Drivers/CMSIS/subdir.mk
-include Application/User/subdir.mk
-include Application/Startup/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

BUILD_ARTIFACT_NAME := Blower_Driver_STSPIN32F0A
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX := 
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME).$(BUILD_ARTIFACT_EXTENSION)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Blower_Driver_STSPIN32F0A.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Blower_Driver_STSPIN32F0A.list \

OBJCOPY_BIN += \
Blower_Driver_STSPIN32F0A.bin \


# All Target
all: main-build

# Main-build Target
main-build: Blower_Driver_STSPIN32F0A.elf secondary-outputs

# Tool invocations
Blower_Driver_STSPIN32F0A.elf: $(OBJS) $(USER_OBJS) C:\acmecpu\stm32_workspace\Blower_Driver_STSPIN32F0A\source\Blower_Driver_STSPIN32F0A\STM32CubeIDE\STM32F031C6TX_FLASH.ld
	arm-none-eabi-gcc -o "Blower_Driver_STSPIN32F0A.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m0 -T"C:\acmecpu\stm32_workspace\Blower_Driver_STSPIN32F0A\source\Blower_Driver_STSPIN32F0A\STM32CubeIDE\STM32F031C6TX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Blower_Driver_STSPIN32F0A.map" -Wl,--gc-sections -static -L../../MCSDK_v5.4.8/MotorControl/lib --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Blower_Driver_STSPIN32F0A.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Blower_Driver_STSPIN32F0A.list"
	@echo 'Finished building: $@'
	@echo ' '

Blower_Driver_STSPIN32F0A.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "Blower_Driver_STSPIN32F0A.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents fail-specified-linker-script-missing warn-no-linker-script-specified
.SECONDARY:

-include ../makefile.targets
