set( CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../Toolchain-rpi.cmake )

cmake_minimum_required( VERSION 2.6 )
project( controller_rpi )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -Og -std=c11 -D_GNU_SOURCE -D_BSD_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -D_GNU_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb3 -Og -std=c++11 -DGE_LINUX -DGE_EGL -fpermissive -Wno-pmf-conversions" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/opt/vc/lib" )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../libcontroller libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller/links )
include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )
include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )

add_custom_target( libluajit_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_luajit.sh ${CMAKE_BINARY_DIR} ${CMAKE_C_COMPILER} ${CMAKE_ASM_COMPILER} ${CROSS}strip "-m32" )

include_directories( ${CMAKE_SOURCE_DIR}/../external/OpenMaxIL++/include )
include_directories( ${CMAKE_SOURCE_DIR}/../external/OpenMaxIL++/MMAL++/include )
add_subdirectory( ${CMAKE_SOURCE_DIR}/../external/OpenMaxIL++ ${CMAKE_CURRENT_BINARY_DIR}/OpenMaxIL++ )
add_subdirectory( ${CMAKE_SOURCE_DIR}/../external/OpenMaxIL++/MMAL++ ${CMAKE_CURRENT_BINARY_DIR}/OpenMaxIL++/MMAL++ )

add_executable( controller_rpi
				main.cpp
				Config.cpp
				ControllerPi.cpp
				Stream.cpp
				RendererHUD.cpp
				RendererHUDNeo.cpp
				RendererHUDClassic.cpp
				ui/PageMain.cpp
				ui/PagePID.cpp
				ui/PageCalibrate.cpp
				ui/PageNetwork.cpp
				ui/PageSettings.cpp
				ui/Globals.cpp
				DecodedImage.cpp
				decode.cpp
				ADCs/MCP320x.cpp
				)

add_dependencies( controller_rpi controller )
add_dependencies( controller_rpi libluajit_static )
add_dependencies( controller_rpi "OpenMaxIL++" )
add_dependencies( controller_rpi "MMAL++" )

target_link_libraries( controller_rpi -L./ controller "OpenMaxIL++" "MMAL++" libluajit_static.a -lbcui -lgammaengine -lpng -ljpeg -lfreetype -lmad -lrt -lwiringPi -ldl -lopenmaxil -lGLESv2 -lEGL -lbcm_host -lvcos -lvchiq_arm -liw rawwifi -lpcap -lz pthread )

install( TARGETS controller_rpi RUNTIME DESTINATION bin )
