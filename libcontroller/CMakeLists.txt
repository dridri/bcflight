cmake_minimum_required( VERSION 2.6 )
project( controller )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -D_GNU_SOURCE -D_BSD_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++11 -fpermissive -Wno-pmf-conversions" )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../librawwifi ${CMAKE_BINARY_DIR}/librawwifi )
include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )

add_library( controller
			 Controller.cpp
			 Thread.cpp
			 links/Link.cpp
			 links/Socket.cpp
			 links/RawWifi.cpp
			)

add_dependencies( controller rawwifi )

target_link_libraries( controller -liw ${CMAKE_BINARY_DIR}/librawwifi/librawwifi.a -lpcap -lz )
