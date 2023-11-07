cmake_minimum_required( VERSION 3.5 )
INCLUDE( CheckSymbolExists )
project( luacore )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g3 -std=gnu++14" ) # -Wno-pmf-conversions
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g3" )
SET( CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE" )

add_library( luacore STATIC
			 src/LuaInterface.cpp
			 src/Lua.cpp
)

target_link_libraries( luacore -ldl )
