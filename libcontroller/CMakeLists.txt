cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )
project( controller )

if ( NOT RAWWIFI_ON )
	SET( RAWWIFI_ON 1 CACHE INTERNAL "RAWWIFI_ON" )
endif()

option(rawwifi "rawwifi")
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
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++14 -fpermissive -Wno-pmf-conversions" )

if ( ${USE_MINISTD} MATCHES TRUE )
	include_directories( ${CMAKE_SOURCE_DIR}/../libministd/include )
endif()

add_definitions( -DSTD=std )

include_directories( ${CMAKE_SOURCE_DIR}/../libdebug )

if ( RAWWIFI_ON MATCHES 1 )
	add_subdirectory( ../librawwifi++ ${CMAKE_BINARY_DIR}/librawwifi )
	include_directories( ../librawwifi++ )
endif()

if ( "${CMAKE_SYSTEM_NAME}" MATCHES "Linux" )
	include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )
elseif( "${CMAKE_SYSTEM_NAME}" MATCHES "Generic" )
	include_directories( ${CMAKE_SOURCE_DIR}/../external/liblua )
endif()
include_directories( ${CMAKE_SOURCE_DIR}/../libluacore/src )

add_library( controllerbase STATIC
			 ControllerBase.cpp
			)

file( GLOB_RECURSE LINKS_SOURCES links/*.cpp links/*.c links/*/*.cpp links/*/*.c )
if ( ${board} MATCHES OFF OR ${board} MATCHES "generic" OR ${board} MATCHES "pc" )
	list( REMOVE_ITEM LINKS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/links/nRF24L01.cpp )
	list( REMOVE_ITEM LINKS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/links/SX127x.cpp )
endif()
add_library( controller STATIC
			 Controller.cpp
			 Thread.cpp
			 ${LINKS_SOURCES}
			)

if ( ${RAWWIFI_ON} MATCHES 1 )
	add_dependencies( controllerbase rawwifi )
	target_link_libraries( controllerbase ${CMAKE_BINARY_DIR}/librawwifi/librawwifi.a -lz -liw )
else()
# 	target_link_libraries( controllerbase -lz -liw )
endif()

target_link_libraries( controller controllerbase )
