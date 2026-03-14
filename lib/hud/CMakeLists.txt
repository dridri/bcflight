cmake_minimum_required( VERSION 3.10 )

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )

project( hud )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -Wno-unused-result -D_DEFAULT_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++17 -fpermissive -Wno-pmf-conversions" )

add_submodule( maths )

if ( NOT FREETYPE_INCLUDE_DIRS )
	find_package( PkgConfig REQUIRED )
	pkg_search_module( FREETYPE REQUIRED freetype2 )
endif()
message( STATUS "Freetype include dirs: ${FREETYPE_INCLUDE_DIRS}" )
message( STATUS "Freetype libraries: ${FREETYPE_LIBRARIES}" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../maths )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../luacore/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../boards/${board} )
include_directories( ${NL_INCLUDE_DIR} )
include_directories( ${FREETYPE_INCLUDE_DIRS} )

set( SRCS
		RendererHUD.cpp
		RendererHUDNeo.cpp
		Font.cpp
	)

add_library( hud STATIC ${SRCS} )
target_link_libraries( hud ${FREETYPE_LIBRARIES} )

set( LIBS ${LIBS} ${FREETYPE_LIBRARIES} )
set( LIBS ${LIBS} PARENT_SCOPE )
set( LIBS ${LIBS} CACHE INTERNAL "" )
