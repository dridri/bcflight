cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../CMakeModules" )
include( add_submodule )
project( links )


if ( NOT RAWWIFI_ON )
	SET( RAWWIFI_ON 1 CACHE INTERNAL "RAWWIFI_ON" )
endif()

option( rawwifi "rawwifi" )

if ( ${rawwifi} MATCHES 1 OR ${rawwifi} MATCHES 0 )
	SET( RAWWIFI_ON ${rawwifi} )
	SET( RAWWIFI_ON ${rawwifi} CACHE INTERNAL "RAWWIFI_ON" )
endif()

if ( WIN32 OR MINGW )
	message( "Win32 detected, disabling rawwifi" )
	set( RAWWIFI_ON 0 )
	SET( RAWWIFI_ON 0 CACHE INTERNAL "RAWWIFI_ON" )
endif()

if ( ${RAWWIFI_ON} MATCHES 0 )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_RAWWIFI" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_RAWWIFI" )
endif()


set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -D_DEFAULT_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++17 -fpermissive -Wno-pmf-conversions" )

add_submodule( debug )
add_submodule( boards )
add_submodule( maths )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../luacore/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../external/LuaJIT-2.0.4/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../boards/common )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../boards/${board} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../controller )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../debug )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../maths )

if ( RAWWIFI_ON MATCHES 1 )
	add_submodule( rawwifi++ )
	include_directories( ../rawwifi++ )
endif()

file( GLOB_RECURSE LINKS_SOURCES *.cpp *.c */*.cpp */*.c )
if ( ${board} MATCHES OFF OR ${board} MATCHES "generic" OR ${board} MATCHES "pc" )
	list( REMOVE_ITEM LINKS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/nRF24L01.cpp )
	list( REMOVE_ITEM LINKS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/SX127x.cpp )
endif()
add_library( links STATIC ${LINKS_SOURCES} )

if ( ${RAWWIFI_ON} MATCHES 1 )
	add_dependencies( controllerbase rawwifi )
	target_link_libraries( links debuglib luacore rawwifi -lz -liw )
else()
	target_link_libraries( links debuglib luacore -liw )
endif()
