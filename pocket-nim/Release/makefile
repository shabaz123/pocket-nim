################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Startup/subdir.mk
-include Libraries/XMCLib/src/subdir.mk
-include Libraries/Newlib/subdir.mk
-include Dave/Generated/SYSTIMER/subdir.mk
-include Dave/Generated/PWM_CCU4/subdir.mk
-include Dave/Generated/I2C_MASTER/subdir.mk
-include Dave/Generated/GLOBAL_CCU4/subdir.mk
-include Dave/Generated/DIGITAL_IO/subdir.mk
-include Dave/Generated/CPU_CTRL_XMC1/subdir.mk
-include Dave/Generated/CLOCK_XMC1/subdir.mk
-include Dave/Generated/subdir.mk
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

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
pocket-nim.hex \

SECONDARY_LIST += \
pocket-nim.lst \

SECONDARY_SIZE += \
pocket-nim.siz \


LIST_FILES = $(addsuffix .o.lst, $(basename $(OBJS))) 
MAP_FILE = "pocket-nim.map"

TOOLCHAIN_ROOT=C:/DAVEv4-64Bit/DAVE-4.3.2/eclipse/ARM-GCC-49
PROJECT_LOC=C:/c_root/electronics/DAVE_work_2019/pocket-nim

# All Target
all: pocket-nim.elf secondary-outputs

# Tool invocations
pocket-nim.elf: $(OBJS) ../linker_script.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM-GCC C Linker'
	$(file >objects.rsp,$(OBJS))
	"$(TOOLCHAIN_ROOT)/bin/arm-none-eabi-gcc" -T"../linker_script.ld" -nostartfiles -Xlinker --gc-sections -specs=nano.specs -specs=nosys.specs -Wl,-Map,$(MAP_FILE) -mcpu=cortex-m0 -mthumb -o "pocket-nim.elf" "@objects.rsp" $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo.
pocket-nim.hex: pocket-nim.elf
	@echo 'Invoking: ARM-GCC Create Flash Image'
	"$(TOOLCHAIN_ROOT)/bin/arm-none-eabi-objcopy" -O ihex "pocket-nim.elf" "pocket-nim.hex"
	@echo 'Finished building: $@'
	@echo.
pocket-nim.lst: pocket-nim.elf
	@echo 'Invoking: ARM-GCC Create Listing'
	"$(TOOLCHAIN_ROOT)/bin/arm-none-eabi-objdump" -h -S "pocket-nim.elf" > "pocket-nim.lst"
	@echo 'Finished building: $@'
	@echo.
pocket-nim.siz: pocket-nim.elf
	@echo 'Invoking: ARM-GCC Print Size'
	"$(TOOLCHAIN_ROOT)/bin/arm-none-eabi-size" --format=berkeley "pocket-nim.elf"
	@echo 'Finished building: $@'
	@echo.
# Other Targets
clean:
	-$(RM) $(OBJS)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(ARM_GCC_FOR_XMC_LINKER_OUTPUT_TYPE_OUTPUTS)$(S_DEPS)$(S_UPPER_DEPS)$(C_DEPS) pocket-nim.elf $(LIST_FILES) $(MAP_FILE) objects.rsp
	-@echo.
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
