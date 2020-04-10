cmake_minimum_required( VERSION 2.6 )
project( ministd )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -std=gnu11 -Wno-unused-result" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )

set( SRCS
	src/string.cpp
)

add_library( ministd STATIC ${SRCS} )
