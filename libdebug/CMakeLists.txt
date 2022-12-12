cmake_minimum_required( VERSION 2.6 )
INCLUDE( CheckSymbolExists )
project( libdebug )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g3 -std=gnu++14 -Wno-pmf-conversions" )

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

add_library( libdebug STATIC Debug.cpp )
target_link_libraries( libdebug ${CMAKE_THREAD_LIBS_INIT} )
