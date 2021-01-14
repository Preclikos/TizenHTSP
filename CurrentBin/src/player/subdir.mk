################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/player/elementary_stream_packet.cc \
../src/player/es_htsp_player_controller.cc \
../src/player/ffmpeg_demuxer.cc \
../src/player/player_listeners.cc 

OBJS += \
./src/player/elementary_stream_packet.po \
./src/player/es_htsp_player_controller.po \
./src/player/ffmpeg_demuxer.po \
./src/player/player_listeners.po 


# Each subdirectory must supply rules for building sources it contributes
src/player/%.po: ../src/player/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: NaCl C++ compiler'
	pnacl-clang++ -I"C:\Users\jan.husak\pepper_47/include" -I"C:\Users\jan.husak\Downloads\hello_world_cpp\third_party\include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


