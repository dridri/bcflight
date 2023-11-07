cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
project( hud )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -std=gnu11 -Wno-unused-result" )

include_directories( ${NL_INCLUDE_DIR} )

if ( "${CMAKE_SYSTEM_NAME}" MATCHES "Linux" )
	include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )
elseif( "${CMAKE_SYSTEM_NAME}" MATCHES "Generic" )
	include_directories( ${CMAKE_SOURCE_DIR}/../external/liblua )
endif()
include_directories( ${CMAKE_SOURCE_DIR}/../libluacore/src )

set( SRCS
		RendererHUD.cpp
		RendererHUDNeo.cpp
	)

add_library( hud STATIC ${SRCS} )
