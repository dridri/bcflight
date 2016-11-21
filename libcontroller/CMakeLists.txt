cmake_minimum_required( VERSION 2.6 )
project( controller )

set( RAWWIFI_ON 1 )

if ( WIN32 OR MINGW )
	set( RAWWIFI_ON 0 )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_RAWWIFI" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_RAWWIFI" )
endif()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -D_GNU_SOURCE -D_BSD_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++11 -fpermissive -Wno-pmf-conversions" )

if ( RAWWIFI_ON )
	add_subdirectory( ${CMAKE_SOURCE_DIR}/../librawwifi ${CMAKE_BINARY_DIR}/librawwifi )
	include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )
endif()

add_library( controller STATIC
			 Controller.cpp
			 Thread.cpp
			 links/Link.cpp
			 links/Socket.cpp
			 links/RawWifi.cpp
			)

if ( RAWWIFI_ON )
	add_dependencies( controller rawwifi )
	target_link_libraries( controller ${CMAKE_BINARY_DIR}/librawwifi/librawwifi.a -lz )
else()
	target_link_libraries( controller -lz )
endif()
