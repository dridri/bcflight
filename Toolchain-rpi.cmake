SET( CMAKE_SYSTEM_NAME Linux )
SET( CMAKE_SYSTEM_VERSION 1 )

SET( CROSS arm-linux-gnueabihf- )
SET( CMAKE_C_COMPILER arm-linux-gnueabihf-gcc )
SET( CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++ )
SET( CMAKE_ASM_COMPILER arm-linux-gnueabihf-as )

execute_process(
	COMMAND bash -c "dirname $(dirname $(whereis arm-linux-gnueabihf-gcc | cut -d':' -f2 | tr -d ' '))"
	OUTPUT_VARIABLE ROOT_PATH
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

SET( CMAKE_FIND_ROOT_PATH ${ROOT_PATH} )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${ROOT_PATH}/arm-linux-gnueabihf/lib/arm-linux-gnueabihf")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

SET( FREETYPE_INCLUDE_DIRS "${CMAKE_FIND_ROOT_PATH}/include/freetype2" )
SET( DRM_INCLUDE_DIRS "${CMAKE_FIND_ROOT_PATH}/include/libdrm" )

SET( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
SET( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
SET( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
