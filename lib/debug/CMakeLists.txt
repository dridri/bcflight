cmake_minimum_required( VERSION 3.5 )
INCLUDE( CheckSymbolExists )
project( debuglib )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g3 -std=c++17 -Wno-pmf-conversions" )

find_package( Threads REQUIRED )
SET( CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE" )
SET( CMAKE_REQUIRED_LIBRARIES "pthread" )
CHECK_SYMBOL_EXISTS( pthread_getname_np pthread.h HAVE_PTHREAD_GETNAME_NP )
CHECK_SYMBOL_EXISTS( pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP )
if ( HAVE_PTHREAD_GETNAME_NP )
	add_definitions( -DHAVE_PTHREAD_GETNAME_NP )
endif()
if ( HAVE_PTHREAD_SETNAME_NP )
	add_definitions( -DHAVE_PTHREAD_SETNAME_NP )
endif()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_library( debuglib STATIC Debug.cpp )
target_link_libraries( debuglib ${CMAKE_THREAD_LIBS_INIT} )
