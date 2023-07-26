execute_process( COMMAND uname -p OUTPUT_VARIABLE BUILD_ARCH )

set( TARGET_CPU_BITS 32 )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEFAULT_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib")
set(CMAKE_LINKER_FLAGS "${CMAKE_LD_FLAGS}")
include_directories( /opt/vc/include )
set( BOARD_LIBS -lpigpio -lrt -lpthread -ldl -lz -liw )

set( BOARD_LIBS ${BOARD_LIBS} Qt5Widgets Qt5Gui Qt5PrintSupport Qt5Core -lGLESv2 -lEGL )
get_filename_component( GCC_FULL_PATH ${CMAKE_C_COMPILER} ABSOLUTE )
get_filename_component( BINARY_FULL_PATH ${GCC_FULL_PATH} DIRECTORY )

# set( UIC ${BINARY_FULL_PATH}/uic )
# set( RCC ${BINARY_FULL_PATH}/rcc )
# set( MOC ${BINARY_FULL_PATH}/moc )
set( UIC uic )
set( RCC rcc )
set( MOC moc )
