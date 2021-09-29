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
	pnacl-clang++ -I"/home/preclikos/pepper_63/include" -I"/home/preclikos/pepper_63/include/newlib" -I"/home/preclikos/tizen-repos/TizenHTSP/third_party/include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


