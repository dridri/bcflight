set( CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../Toolchain-rpi.cmake )

cmake_minimum_required( VERSION 2.6 )
project( controller_rpi )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -std=c++11 -DGE_LINUX -DGE_EGL -fpermissive -Wno-pmf-conversions" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib" )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../librawwifi ${CMAKE_BINARY_DIR}/librawwifi )
include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )

add_executable( controller_rpi
				main.cpp
				Controller.cpp
				Stream.cpp
				RendererHUD.cpp
				ADCs/MCP320x.cpp
				ui/PageMain.cpp
				ui/PageCalibrate.cpp
				ui/PageNetwork.cpp
				ui/PageSettings.cpp
				ui/Globals.cpp
				DecodedImage.cpp
				decode.cpp
				)

add_dependencies( controller_rpi rawwifi )

target_link_libraries( controller_rpi -lbcui -lgammaengine -lpng -ljpeg -lfreetype -lmad -lrt -ldl -lopenmaxil -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -liw ${CMAKE_BINARY_DIR}/librawwifi/librawwifi.a -lpcap -lz )

install( TARGETS controller_rpi RUNTIME DESTINATION bin )
