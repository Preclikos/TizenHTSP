################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/handler.cc 

CPP_SRCS += \
../src/hello_world.cpp 

OBJS += \
./src/handler.po \
./src/hello_world.po 


# Each subdirectory must supply rules for building sources it contributes
src/%.po: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: NaCl C++ compiler'
	pnacl-clang++ -I"C:\Users\jan.husak\pepper_47/include" -I"C:\Users\jan.husak\pepper_47/include/newlib" -I"C:\Users\jan.husak\Downloads\hello_world_cpp\third_party\include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.po: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NaCl C++ compiler'
	pnacl-clang++ -I"C:\Users\jan.husak\pepper_47/include" -I"C:\Users\jan.husak\pepper_47/include/newlib" -I"C:\Users\jan.husak\Downloads\hello_world_cpp\third_party\include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


