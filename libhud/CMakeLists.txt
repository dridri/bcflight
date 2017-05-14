cmake_minimum_required( VERSION 2.6 )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
project( hud )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -std=gnu11 -Wno-unused-result" )

include_directories( ${NL_INCLUDE_DIR} )

set( SRCS
		RendererHUD.cpp
		RendererHUDNeo.cpp
	)

add_library( hud STATIC ${SRCS} )
