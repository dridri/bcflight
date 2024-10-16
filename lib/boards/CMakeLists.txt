cmake_minimum_required( VERSION 3.5 )

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../CMakeModules)
include( add_submodule )

SET( board "rpi" CACHE STRING "board" )
SET( variant "" CACHE STRING "variant" )
option( board "board" )

project( boards )

message( STATUS "Board type : ${board}" )


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

DIRLIST( BOARDS ${CMAKE_CURRENT_SOURCE_DIR} "" )

if ( ${board} MATCHES OFF )
	message( FATAL_ERROR "You must specify a target board.\nAvailable boards are :\n${BOARDS}" )
endif()

list( FIND BOARDS ${board} board_found )
if ( ${board_found} LESS 0 )
	message( FATAL_ERROR "Board ${board} not supported.\nAvailable boards are :\n${BOARDS}" )
endif()

include( ${board}/board.cmake )

add_submodule( debug )
add_submodule( luacore )
add_submodule( maths )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../debug )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../luacore/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../external/LuaJIT-2.0.4/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../maths )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/common )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/${board} )

# if ( ${board} MATCHES "rpi" )
# 	set( CAMERAS Raspicam )
# 	if ( ${camera} MATCHES OFF )
# 		set( camera Raspicam )
# 	endif()
# endif()
# list( FIND CAMERAS ${camera} camera_found )
# if ( NOT ${camera} MATCHES OFF AND NOT ${camera} MATCHES 0 AND ${camera_found} LESS 0 )
# 	message( FATAL_ERROR "Camera ${camera} not supported.\nAvailable cameras for this board are :\n${CAMERAS}" )
# endif()


if ( ${board} MATCHES "generic" AND NOT ${TARGET_CPU_BITS} MATCHES OFF )
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m${TARGET_CPU_BITS}" )
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${TARGET_CPU_BITS}" )
endif()

file( GLOB BOARD_SOURCES_AUTO ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${board}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${board}/*.c )
add_library( boards STATIC ${BOARD_SOURCES_AUTO} )
target_include_directories( boards PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${board}" )
