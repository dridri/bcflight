cmake_minimum_required( VERSION 3.5 )
INCLUDE( CheckSymbolExists )
project( luacore )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g3 -std=c++17" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g3" )
SET( CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../external/LuaJIT-2.0.4/src )

add_definitions( -DTARGET_CPU_BITS=${TARGET_CPU_BITS} )
add_definitions( -DLUAJIT_VERSION )
if ( (NOT "${TARGET_CPU_BITS}" MATCHES OFF) AND NOT "${TARGET_CPU_BITS}" STREQUAL "${HOST_CPU_BITS}" )
	set( LUA_HOST_CC_OPTIONS "-m${TARGET_CPU_BITS}" )
else()
	set( LUA_HOST_CC_OPTIONS "" )
endif()
add_custom_target( libluajit_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_luajit.sh "${CMAKE_BINARY_DIR}" "${CMAKE_C_COMPILER}" "${CMAKE_ASM_COMPILER}" "${CROSS}strip" "${LUA_HOST_CC_OPTIONS}" "${CMAKE_C_FLAGS} -Wno-misleading-indentation" )

add_library( luacore STATIC
			 src/LuaInterface.cpp
			 src/Lua.cpp
)

add_dependencies( luacore libluajit_static )

target_link_libraries( luacore libluajit_static.a -ldl )
