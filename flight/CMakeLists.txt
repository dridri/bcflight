cmake_minimum_required( VERSION 3.5 )

include(ProcessorCount)
ProcessorCount(NCPU)

set( VERSION_MAJOR 0 )
set( VERSION_MINOR 1 )
execute_process( COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE )
execute_process( COMMAND getconf LONG_BIT OUTPUT_VARIABLE HOST_CPU_BITS OUTPUT_STRIP_TRAILING_WHITESPACE )

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../CMakeModules)

SET( board "rpi" CACHE STRING "board" )
SET( variant "" CACHE STRING "variant" )
SET( socket 1 CACHE BOOL "socket" )
SET( rawwifi 0 CACHE BOOL "rawwifi" )
SET( hud 1 CACHE BOOL "hud" )

option(board "board")
option(camera "camera")
option(debug "debug")
option(socket "socket")
option(rawwifi "rawwifi")

include( config.cmake )
if ( ${BUILD_links} MATCHES 0 OR ${BUILD_links} MATCHES OFF )
	file(GLOB children RELATIVE ${CMAKE_SOURCE_DIR}/links ${CMAKE_SOURCE_DIR}/links/*)
	set(dirlist "")
	foreach(child ${children})
		if ( "${child}" MATCHES ".cpp" AND NOT "${child}" MATCHES "Link.cpp" )
			get_filename_component( child ${child} NAME_WE )
			set( DBUILD_${child} 0 )
			remove_definitions( -DBUILD_${child} )
		endif()
	endforeach()
endif()

if ( NOT ${board} MATCHES OFF AND NOT ${board} MATCHES "generic" )
	set( CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../Toolchain-${board}.cmake )
endif()

enable_language( ASM )
project( flight )

find_package( FFmpeg COMPONENTS AVFORMAT AVUTIL AVCODEC REQUIRED )
find_package( PkgConfig REQUIRED )
pkg_search_module( FFTW REQUIRED fftw3f IMPORTED_TARGET )
set( LIBS ${LIBS} ${FFMPEG_LIBRARIES} -lfftw3f )


macro( DIRLIST result curdir removes )
	file(GLOB children RELATIVE ${curdir} ${curdir}/*)
	set(dirlist "")
	foreach(child ${children})
		if ( NOT "${child}" MATCHES ".md" )
			string(REPLACE ".cpp" "" child ${child})
			string(REPLACE ".h" "" child ${child})
			list(APPEND dirlist ${child})
		endif()
	endforeach()
	if ( NOT ${removes} MATCHES OFF )
		list(REMOVE_ITEM dirlist ${removes})
	endif()
	list(REMOVE_DUPLICATES dirlist)
	set(${result} ${dirlist})
endmacro()

DIRLIST( BOARDS ${CMAKE_SOURCE_DIR}/boards "" )

if ( ${board} MATCHES OFF )
	message( FATAL_ERROR "You must specify a target board.\nAvailable boards are :\n${BOARDS}" )
endif()

list( FIND BOARDS ${board} board_found )
if ( ${board_found} LESS 0 )
	message( FATAL_ERROR "Board ${board} not supported.\nAvailable boards are :\n${BOARDS}" )
endif()

if ( ${board} MATCHES "rpi" )
	set( CAMERAS Raspicam )
	if ( ${camera} MATCHES OFF )
		set( camera Raspicam )
	endif()
endif()

if ( ${camera} MATCHES OFF )
# 	message( FATAL_ERROR "You must specify a camera.\nAvailable cameras for this board are :\n${CAMERAS}" )
endif()

list( FIND CAMERAS ${camera} camera_found )
if ( NOT ${camera} MATCHES OFF AND NOT ${camera} MATCHES 0 AND ${camera_found} LESS 0 )
	message( FATAL_ERROR "Camera ${camera} not supported.\nAvailable cameras for this board are :\n${CAMERAS}" )
endif()

include( boards/${board}/board.cmake )
add_definitions( -DSYSTEM_NAME=${CMAKE_SYSTEM_NAME} -DSYSTEM_NAME_${CMAKE_SYSTEM_NAME} )
add_definitions( -DBOARD="${board}" -DBOARD_${board} -DVARIANT="${variant}" -DVARIANT_${variant} )
add_definitions( -DVERSION_STRING="bcflight_${board}_${VERSION_MAJOR}.${VERSION_MINOR}_${GIT_HASH}" )
if ( ${USE_MINISTD} MATCHES TRUE )
	add_definitions( -DSTD=ministd )
else()
	add_definitions( -DSTD=std )
endif()
if ( NOT ${camera} MATCHES OFF AND NOT ${camera} MATCHES 0 )
	add_definitions( -DCAMERA=${camera} -DCAMERA_INCLUDE="${camera}.h" )
endif()

if ( ${TARGET_LINUX} MATCHES "1" )
	include_directories(${LIBCAMERA_INCLUDE_DIRS})
else()
	set( socket 0 )
	set( rawwifi 0 )
	set( hud 0 )
endif()
add_definitions( -DBUILD_SOCKET=${socket} )
add_definitions( -DBUILD_RAWWIFI=${rawwifi} )

if ( ${board} MATCHES "generic" AND NOT ${TARGET_CPU_BITS} MATCHES OFF )
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m${TARGET_CPU_BITS}" )
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${TARGET_CPU_BITS}" )
	add_definitions( -DTARGET_CPU_BITS=${TARGET_CPU_BITS} )
endif()
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wuninitialized -std=gnu11 -fgnu89-inline" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wuninitialized -std=gnu++17 -Wno-pmf-conversions -Wno-unused-result -Wno-unused -Wno-address-of-packed-member -Wno-nonnull-compare" )
set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -Wall -std=gnu11" )
set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -Wall -std=gnu11" )

if ( "${debug}" MATCHES "yes" OR "${debug}" MATCHES "1" )
	message( "-- WARNING : Debug enabled" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0" )
	set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g3 -O0" )
	set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -g3 -O0" )
else()
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -Os" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -Os" )
	set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -s -Os" )
	set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -s -Os" )
endif()

include_directories( ${CMAKE_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR} )
include_directories( . )
include_directories( boards/common )
include_directories( boards/${board} )
include_directories( stabilizer )
include_directories( console )
include_directories( frames )
include_directories( links )
include_directories( motors )
include_directories( sensors )
include_directories( peripherals )
include_directories( video )
include_directories( audio )

if ( "${rawwifi}" MATCHES 1 )
	SET( RAWWIFI_ON ${rawwifi} CACHE INTERNAL "RAWWIFI_ON" )
	include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi++ )
	set( LIBS ${LIBS} rawwifi ${NLGENL_LIBRARY} ${NLROUTE_LIBRARY} ${NL_LIBRARY} )
endif()
# 	add_subdirectory( ${CMAKE_SOURCE_DIR}/../librawwifi ${CMAKE_BINARY_DIR}/librawwifi )
# 	include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )
# 	set( LIBS ${LIBS} ${CMAKE_BINARY_DIR}/librawwifi/librawwifi.a -lpcap ${NLGENL_LIBRARY} ${NLROUTE_LIBRARY} ${NL_LIBRARY} )
# endif()

file( GLOB BOARD_SOURCES_AUTO boards/common/*.cpp boards/${board}/*.cpp boards/${board}/*.c )
if ( ${camera} MATCHES OFF OR ${camera} MATCHES 0 )
	list( FILTER BOARD_SOURCES_AUTO EXCLUDE REGEX ".*GL.*" )
endif()
file( GLOB BOARD_HEADERS_AUTO boards/common/*.h boards/${board}/*.h )
file( GLOB CONSOLE_SOURCES console/*.cpp )
file( GLOB CONSOLE_HEADERS console/*.h )
file( GLOB FRAMES_SOURCES frames/*.cpp )
file( GLOB FRAMES_HEADERS frames/*.h )
file( GLOB_RECURSE LINKS_SOURCES links/*.cpp links/*/*.cpp links/*/*.c )
file( GLOB_RECURSE LINKS_HEADERS links/*.h links/*/*.h )
file( GLOB MOTORS_SOURCES motors/*.cpp )
file( GLOB MOTORS_HEADERS motors/*.h )
file( GLOB_RECURSE SENSORS_SOURCES sensors/*.cpp )
file( GLOB_RECURSE SENSORS_HEADERS sensors/*.h )
file( GLOB PERIPHERALS_SOURCES peripherals/*.cpp )
file( GLOB PERIPHERALS_HEADERS peripherals/*.h )
file( GLOB VIDEO_SOURCES video/*.cpp )
file( GLOB VIDEO_HEADERS video/*.h )
file( GLOB AUDIO_SOURCES audio/*.cpp )
file( GLOB AUDIO_HEADERS audio/*.h )
file( GLOB STABILIZER_SOURCES stabilizer/*.cpp )
file( GLOB STABILIZER_HEADERS stabilizer/*.h )

file( GLOB HEADERS ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/../libcontroller/ControllerBase.h )
list( APPEND HEADERS ${BOARD_HEADERS_AUTO} )

set( SOURCES
BlackBox.cpp
	Main.cpp
	Config.cpp
	Console.cpp
	Matrix.cpp
	Quaternion.cpp
	${CMAKE_BINARY_DIR}/lua_init.cpp )

if ( ${BUILD_controller} MATCHES 1 )
	list( APPEND SOURCES Controller.cpp )
endif()
if ( ${BUILD_power} MATCHES 1 )
	list( APPEND SOURCES PowerThread.cpp )
endif()

list( APPEND SOURCES ${BOARD_SOURCES} )
list( APPEND SOURCES ${BOARD_SOURCES_AUTO} )
if ( ${BUILD_stabilizer} MATCHES 1 )
	list( APPEND SOURCES ${STABILIZER_SOURCES} )
	list( APPEND HEADERS ${STABILIZER_HEADERS} )
endif()
if ( ${BUILD_console} MATCHES 1 )
	list( APPEND SOURCES ${CONSOLE_SOURCES} )
	list( APPEND HEADERS ${CONSOLE_HEADERS} )
endif()
if ( ${BUILD_frames} MATCHES 1 )
	list( APPEND SOURCES ${FRAMES_SOURCES} )
	list( APPEND HEADERS ${FRAMES_HEADERS} )
endif()
if ( ${BUILD_links} MATCHES 1 )
	list( APPEND SOURCES ${LINKS_SOURCES} )
	list( APPEND HEADERS ${LINKS_HEADERS} )
endif()
if ( ${BUILD_motors} MATCHES 1 )
	list( APPEND SOURCES ${MOTORS_SOURCES} )
	list( APPEND HEADERS ${MOTORS_HEADERS} )
endif()
if ( ${BUILD_sensors} MATCHES 1 )
	list( APPEND SOURCES ${SENSORS_SOURCES} )
	list( APPEND HEADERS ${SENSORS_HEADERS} )
endif()
if ( ${BUILD_peripherals} MATCHES 1 )
	list( APPEND SOURCES ${PERIPHERALS_SOURCES} )
	list( APPEND HEADERS ${PERIPHERALS_HEADERS} )
endif()
if ( ${BUILD_video} MATCHES 1 )
	list( APPEND SOURCES ${VIDEO_SOURCES} )
	list( APPEND HEADERS ${VIDEO_HEADERS} )
endif()
if ( ${BUILD_audio} MATCHES 1 )
	list( APPEND SOURCES ${AUDIO_SOURCES} )
	list( APPEND HEADERS ${AUDIO_HEADERS} )
endif()

set_source_files_properties( ${CMAKE_BINARY_DIR}/lua_init.cpp PROPERTIES GENERATED TRUE )
add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/lua_init.cpp DEPENDS ${CMAKE_SOURCE_DIR}/../libluacore/classes.auto.lua ${HEADERS} COMMAND ${CMAKE_SOURCE_DIR}/../libluacore/classes.auto.lua --init=lua_init --output="${CMAKE_BINARY_DIR}/lua_init.cpp" ${HEADERS} )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../libdebug ${CMAKE_CURRENT_BINARY_DIR}/libdebug )
include_directories( ${CMAKE_SOURCE_DIR}/../libdebug )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../libcontroller ${CMAKE_CURRENT_BINARY_DIR}/libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller )

add_executable( flight_unstripped ${SOURCES} )

if ( "${hud}" MATCHES 1 )
	add_definitions( -DBUILD_HUD=1 )
	add_subdirectory( ${CMAKE_SOURCE_DIR}/../libhud ${CMAKE_CURRENT_BINARY_DIR}/libhud )
	include_directories( ${CMAKE_SOURCE_DIR}/../libhud )
	set( LIBS ${LIBS} ${CMAKE_BINARY_DIR}/libhud/libhud.a -lpng )
	add_dependencies( flight_unstripped hud )
endif()

if ( "${CMAKE_SYSTEM_NAME}" MATCHES "Linux" )
	add_definitions( -DLUAJIT_VERSION )
	if ( (NOT ${TARGET_CPU_BITS} MATCHES OFF) AND NOT ${TARGET_CPU_BITS} STREQUAL ${HOST_CPU_BITS} )
		set( LUA_HOST_CC_OPTIONS "-m${TARGET_CPU_BITS}" )
	else()
		set( LUA_HOST_CC_OPTIONS "" )
	endif()
	add_custom_target( libluajit_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_luajit.sh ${CMAKE_BINARY_DIR} ${CMAKE_C_COMPILER} ${CMAKE_ASM_COMPILER} ${CROSS}strip ${LUA_HOST_CC_OPTIONS} "${CMAKE_C_FLAGS} -Wno-misleading-indentation" )
	add_dependencies( flight_unstripped libluajit_static )
	include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )
	set( LIBS ${LIBS} luajit_static )
elseif( "${CMAKE_SYSTEM_NAME}" MATCHES "Generic" )
	# add_definitions( -DLUA_32BITS )
	# set( LUA_CFLAGS "${CMAKE_C_FLAGS} -DLUA_32BITS" )
	add_custom_target( liblua_src COMMAND make -C ${CMAKE_SOURCE_DIR}/../external/liblua liblua.a CC=${CMAKE_C_COMPILER} STRIP=${CROSS}strip XCFLAGS=-DLUAJIT_USE_SYSMALLOC CFLAGS=${LUA_CFLAGS} )
	add_custom_target( liblua DEPENDS liblua_src COMMAND cp ${CMAKE_SOURCE_DIR}/../external/liblua/liblua.a ${CMAKE_BINARY_DIR}/liblua.a )
	add_dependencies( flight_unstripped liblua )
	include_directories( ${CMAKE_SOURCE_DIR}/../external/liblua )
	set( LIBS ${LIBS} ${CMAKE_BINARY_DIR}/liblua.a )
endif()


add_subdirectory( ${CMAKE_SOURCE_DIR}/../libluacore ${CMAKE_CURRENT_BINARY_DIR}/libluacore )
include_directories( ${CMAKE_SOURCE_DIR}/../libluacore/src )
add_definitions( -DLUA_CLASS= -DLUA_EXPORT= )
add_dependencies( luacore libluajit_static )

add_dependencies( flight_unstripped libdebug )
# add_dependencies( flight_unstripped flight_register )
add_dependencies( flight_unstripped controllerbase )
add_dependencies( flight_unstripped luacore )
if ( BOARD_DEPENDENCIES )
	add_dependencies( flight_unstripped ${BOARD_DEPENDENCIES} )
endif()
if ( "${rawwifi}" MATCHES 1 )
	add_dependencies( flight_unstripped rawwifi )
endif()

if ( "${CMAKE_SYSTEM_NAME}" MATCHES "Linux" AND ${BUILD_audio} MATCHES 1 )
	set( LIBS ${LIBS} -lshine )
endif()

message( STATUS "LIBS : ${LIBS} ${BOARD_LIBS}" )
target_link_libraries( flight_unstripped -L./ controllerbase luacore libdebug ${LIBS} ${BOARD_LIBS} )


if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL CMAKE_SYSTEM_PROCESSOR)
	include(FetchContent)
	FetchContent_Declare(
		catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG v3.6.0
	)
	FetchContent_MakeAvailable(catch2)
	enable_testing()
	add_subdirectory( ${CMAKE_SOURCE_DIR}/tests )
else()
    message(STATUS "Skipping test build as we are cross-compiling")
endif()

board_strip()
