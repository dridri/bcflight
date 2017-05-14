execute_process( COMMAND uname -p OUTPUT_VARIABLE BUILD_ARCH )

if ( ${BUILD_ARCH} MATCHES "x86*" )
# 	message( "Cross compilation detected, setting compiler prefix to arm-linux-gnueabihf-" )
# 	set( CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc" )
# 	set( CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++" )
# 	set( CMAKE_AR "arm-linux-gnueabihf-ar" )
endif()

set( TARGET_CPU_BITS 32 )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE -D_GNU_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib")
set(CMAKE_LINKER_FLAGS "${CMAKE_LD_FLAGS}")
set( BOARD_LIBS -lasound -lwiringPi -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lrt -lpthread -ldl -lz -liw )
# set( BOARD_LIBS ${BOARD_LIBS} -lavformat -lavcodec -lavutil )
set( BOARD_LIBS ${BOARD_LIBS} -lgps )
include_directories( /opt/vc/include )

include_directories( ${CMAKE_SOURCE_DIR}/../external/OpenMaxIL++/include )
add_subdirectory( ${CMAKE_SOURCE_DIR}/../external/OpenMaxIL++ ${CMAKE_CURRENT_BINARY_DIR}/OpenMaxIL++ )
set( BOARD_DEPENDENCIES "OpenMaxIL++" )
set( BOARD_LIBS ${BOARD_LIBS} "OpenMaxIL++" )

function( board_strip )
	add_custom_target( flight ALL
						COMMAND arm-linux-gnueabihf-strip -s -o flight flight_unstripped
						DEPENDS flight_unstripped
						COMMENT "Stripping executable"
						VERBATIM
	)
endfunction()
