################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/htsp/HTSPConnection.cpp 

OBJS += \
./src/htsp/HTSPConnection.po 


# Each subdirectory must supply rules for building sources it contributes
src/htsp/%.po: ../src/htsp/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NaCl C++ compiler'
	pnacl-clang++ -I"C:\Users\jan.husak\pepper_47/include" -I"C:\Users\jan.husak\Downloads\hello_world_cpp\third_party\include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


