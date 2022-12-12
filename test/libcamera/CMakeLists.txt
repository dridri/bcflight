cmake_minimum_required( VERSION 2.6 )

project( libcameratest )
set(CMAKE_BUILD_RPATH "/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib")
set(CMAKE_INSTALL_RPATH "/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib")

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -O0 -D_GNU_SOURCE -D_DEFAULT_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb3 -O0 -std=c++17 -DGE_LINUX -DGE_EGL -fpermissive -Wno-pmf-conversions -Wno-psabi" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath-link,/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib" )
set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -L/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib" )
set( CMAKE_LD_PATH "${CMAKE_LD_PATH} -L/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib" )

include_directories( ${CMAKE_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR} )
include_directories( /opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/include/libcamera )
include_directories( /opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/include/libdrm )

add_executable( cam
				simple-cam.cpp
				event_loop.cpp
				# main.cpp
				# core/libcamera_app.cpp
				# core/options.cpp
				# core/post_processor.cpp
				# output/output.cpp
				# preview/preview.cpp
				# encoder/encoder.cpp
				)


target_link_libraries( cam
	-ldrm
	/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib/libcamera.so
	/opt/arm-bcm2708/cross-pi-gcc-10.2.0-2/arm-linux-gnueabihf/lib/libcamera-base.so
	-pthread )
