################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/player/elementary_stream_packet.cc \
../src/player/es_htsp_player_controller.cc \
../src/player/ffmpeg_demuxer.cc \
../src/player/player_listeners.cc \
../src/player/url_loader.cc 

OBJS += \
./src/player/elementary_stream_packet.po \
./src/player/es_htsp_player_controller.po \
./src/player/ffmpeg_demuxer.po \
./src/player/player_listeners.po \
./src/player/url_loader.po 


# Each subdirectory must supply rules for building sources it contributes
src/player/%.po: ../src/player/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: NaCl C++ compiler'
	pnacl-clang++ -I"/home/preclikos/pepper_63/include" -I"/home/preclikos/pepper_63/include/newlib" -I"/home/preclikos/tizen-repos/TizenHTSP/third_party/include" -c -std=gnu++0x -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


