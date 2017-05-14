cmake_minimum_required( VERSION 2.6 )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
project( rawwifi )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -std=gnu11 -Wno-unused-result" )

find_package( LibPCAP REQUIRED )
if ( WIN32 )
else()
	find_package( LibNL REQUIRED )
endif()

include_directories( ${NL_INCLUDE_DIR} )

set( SRCS
			rx_base.c
			rx_weighted.c
			tx.c
			rawwifi.c
			radiotap.c
			hamming.c
			blocks.c
			)

if ( WIN32 )
	set( SRCS ${SRCS} interface_win.c )
	set( NL_LIBRARIES -lversion -liphlpapi -lws2_32 -pthread )
	set( PCAP_LIBRARY -lpacket -lwpcap -lpacket )
else()
	set( SRCS ${SRCS} interface_linux.c )
	set( NL_LIBRARIES ${NL_LIBRARIES} -lrt )
endif()

add_library( rawwifi STATIC ${SRCS} )

target_link_libraries( rawwifi ${PCAP_LIBRARY} ${NL_LIBRARIES} )
