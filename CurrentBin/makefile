################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := python $(NACL_SDK_ROOT)/tools/oshelpers.py rm

# All of the sources participating in the build are defined here
-include sources.mk
-include src/player/subdir.mk
-include src/htsp/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
PEXE += \
hello_world_cpp.pexe \

NACL_EXECUTABLE += \
hello_world_cpp_i686.nexe \
hello_world_cpp_x86-64.nexe \
hello_world_cpp_armv7.nexe \


# All Target
all: hello_world_cpp.nmf

# Tool invocations
hello_world_cpp.nmf: $(NACL_EXECUTABLE)
	@echo 'Building target: $@'
	@echo 'Generating manifest file'
	python "$(NACL_SDK_ROOT)/tools/create_nmf.py" -s ./  -o "hello_world_cpp.nmf" $(NACL_EXECUTABLE)
	@echo 'Finished building target: $@'
	@echo ' '

hello_world_cpp.pexe: $(OBJS)
	@echo 'Invoking: NaCl C++ linker'
	pnacl-clang++ -o "hello_world_cpp.pexe" $(OBJS) -L"C:\Users\jan.husak\Downloads\hello_world_cpp\third_party\lib" -L"C:\Users\jan.husak\pepper_63/lib/pnacl/Debug" -lpthread -lavformat -lavcodec -lavutil -lglibc-compat -lc++ -lnacl_player -lnacl_io -lppapi -lppapi_cpp -lhts
	@echo 'Finished building: $@'
	@echo ' '

hello_world_cpp_i686.nexe hello_world_cpp_x86-64.nexe hello_world_cpp_armv7.nexe: $(PEXE)
	@echo 'Invoking: NaCl Translator'
	pnacl-translate --allow-llvm-bitcode-input -arch $(patsubst hello_world_cpp_%.nexe,%,$(notdir $@))  -o $@ $(PEXE)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(NACL_EXECUTABLE)$(NACL_MANIFEST_OUTPUTS)$(PEXE)$(OBJS) hello_world_cpp.nmf
	-@echo ' '

.PHONY: all clean dependents

-include ../makefile.targets
