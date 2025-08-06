cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../CMakeModules" )
include( add_submodule )
project( sensors )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -D_DEFAULT_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++17 -fpermissive -Wno-pmf-conversions" )

add_submodule( debug )
add_submodule( boards )
add_submodule( maths )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../luacore/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../external/LuaJIT-2.0.4/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../boards/common )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../boards/${board} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../debug )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../maths )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

file( GLOB_RECURSE SENSORS_SOURCES *.cpp *.c */*.cpp */*.c )
add_library( sensors STATIC ${SENSORS_SOURCES} )


target_link_libraries( sensors )
