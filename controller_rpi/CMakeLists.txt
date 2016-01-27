set( CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../Toolchain-rpi.cmake )

cmake_minimum_required( VERSION 2.6 )
project( controller_rpi )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -std=c++11 -DGE_LINUX -DGE_EGL -fpermissive" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib" )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../librawwifi ${CMAKE_BINARY_DIR}/librawwifi )
include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )

add_executable( controller_rpi
				main.cpp
				Controller.cpp
				Stream.cpp
				ui/PageMain.cpp
				ui/PageCalibrate.cpp
				ui/Globals.cpp
				DecodedImage.cpp
				decode.cpp
				)

add_dependencies( controller_rpi rawwifi )

target_link_libraries( controller_rpi -lbcui -lgammaengine -lpng -lz -lrt -ldl -lopenmaxil -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -lpcap ${CMAKE_BINARY_DIR}/librawwifi/librawwifi.a )

install( TARGETS controller_rpi RUNTIME DESTINATION bin )
