################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/htsp/socket/tcpsocket_api.cc 

CPP_SRCS += \
../src/htsp/socket/tcpsocketsync_api.cpp 

OBJS += \
./src/htsp/socket/tcpsocket_api.po \
./src/htsp/socket/tcpsocketsync_api.po 


# Each subdirectory must supply rules for building sources it contributes
src/htsp/socket/%.po: ../src/htsp/socket/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: NaCl C++ compiler'
	pnacl-clang++ -I"C:\Users\jan.husak\pepper_56/include" -I"C:\Users\jan.husak\Downloads\hello_world_cpp\third_party\include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/htsp/socket/%.po: ../src/htsp/socket/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NaCl C++ compiler'
	pnacl-clang++ -I"C:\Users\jan.husak\pepper_56/include" -I"C:\Users\jan.husak\Downloads\hello_world_cpp\third_party\include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


