cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
project( maths )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -Wno-unused-result -D_DEFAULT_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++17 -fpermissive -Wno-pmf-conversions" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../debug )

set( SRCS
	Matrix.cpp
	Quaternion.cpp
)

add_library( maths STATIC ${SRCS} )
