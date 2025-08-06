execute_process( COMMAND uname -p OUTPUT_VARIABLE BUILD_ARCH )

if ( ${BUILD_ARCH} MATCHES "x86*" )
# 	message( "Cross compilation detected, setting compiler prefix to arm-linux-gnueabihf-" )
# 	set( CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc" )
# 	set( CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++" )
# 	set( CMAKE_AR "arm-linux-gnueabihf-ar" )
endif()

set( TARGET_LINUX 1 CACHE INTERNAL "" )
set( TARGET_CPU_BITS 32 CACHE INTERNAL "" )

find_package( Threads REQUIRED )
find_package( GPS REQUIRED )

if ( GPS_FOUND )
	set( BOARD_LIBS ${BOARD_LIBS} ${GPS_LIBRARIES} )
endif()

SET( CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE" )
SET( CMAKE_REQUIRED_LIBRARIES "pthread" )
add_definitions( -DHAVE_PTHREAD_GETNAME_NP )
add_definitions( -DHAVE_PTHREAD_SETNAME_NP )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DEFAULT_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi"  )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vmcs_host/khronos -I/opt/vc/include/interface/vmcs_host/khronos -I/opt/vc/include/interface/khronos/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib/" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vmcs_host/khronos -I/opt/vc/include/interface/vmcs_host/khronos -I/opt/vc/include/interface/khronos/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib" )
set(CMAKE_LD_FLAGS "-mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib" )
set(CMAKE_LINKER_FLAGS "${CMAKE_LD_FLAGS}" )
set( BOARD_LIBS ${BOARD_LIBS} -lpigpio -lbcm_host -lvcos -lvchiq_arm -lrt -lpthread -ldl -lz -ldrm ) #  -lpigpio

if ( BUILD_audio )
	set( BOARD_LIBS ${BOARD_LIBS} -lasound )
endif()

get_filename_component( CROSS_ROOT ${CMAKE_C_COMPILER} DIRECTORY )
include_directories( ${CROSS_ROOT}/../include )
include_directories( ${CROSS_ROOT}/../include/libdrm )
include_directories( ${CROSS_ROOT}/../arm-linux-gnueabihf/include )
include_directories( ${CROSS_ROOT}/../arm-linux-gnueabihf/include/libdrm )
include_directories( /opt/vc/include )

#if ( "${rawwifi}" MATCHES 1 )
	set( BOARD_LIBS ${BOARD_LIBS} -liw -lcamera -lcamera-base -lgps )
#endif()

add_definitions( -DHAVE_LIBIW )


set( BOARD_LIBS ${BOARD_LIBS} -lGLESv2 -lEGL )
set( BOARD_LIBS ${BOARD_LIBS} -ldrm -lgbm )

set( BOARD_LIBS ${BOARD_LIBS} CACHE INTERNAL "" )
set( BOARD_LIBS_PATHS "-L/opt/vc/lib" CACHE INTERNAL "" )

function( board_strip target_source target_dest )
	add_custom_target( ${target_dest} ALL
						COMMAND arm-linux-gnueabihf-strip -s -o ${target_dest} ${target_source}
						DEPENDS ${target_source}
						COMMENT "Stripping executable (${target_source} -> ${target_dest})"
						VERBATIM
	)
endfunction()
