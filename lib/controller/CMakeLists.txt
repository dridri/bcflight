cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../CMakeModules" )
include( add_submodule )
project( controller )


set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -D_DEFAULT_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++17 -fpermissive -Wno-pmf-conversions" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../debug )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../links )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../boards/${board} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../luacore/src )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../external/LuaJIT-2.0.4/src )

add_definitions( -DLUAJIT_VERSION )
add_submodule( debug )
add_submodule( links )
add_submodule( luacore )
add_submodule( boards )

add_library( controllerbase STATIC ControllerBase.cpp )
add_library( controller STATIC Controller.cpp )

add_dependencies( controller debuglib )
add_dependencies( controller links )
add_dependencies( controller luacore )


target_link_libraries( controllerbase links debuglib luacore )
target_link_libraries( controller controllerbase )
