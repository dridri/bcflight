cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
project( rawwifi )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3 -std=c++14 -Wall -Wno-unused-result" )

find_package( PCAP REQUIRED )
if ( WIN32 )
else()
	find_package( NL REQUIRED )
endif()

include_directories( ${NL_INCLUDE_DIR} )

include_directories( ${CMAKE_SOURCE_DIR}/../libdebug )

set( SRCS
	RawWifiRx.cpp
	RawWifiTx.cpp
	RawWifi.cpp
	PcapHandler.cpp
	WifiInterfaceLinux.cpp
	radiotap.c
)

if ( WIN32 )
	# set( SRCS ${SRCS} WifiInterfaceWindows.cpp )
	# set( NL_LIBRARIES -lversion -liphlpapi -lws2_32 -pthread )
	# set( PCAP_LIBRARY -lpacket -lwpcap -lpacket )
else()
	set( SRCS ${SRCS} WifiInterfaceLinux.cpp )
	set( NL_LIBRARIES ${NL_LIBRARIES} -lrt )
endif()

add_library( rawwifi STATIC ${SRCS} )

target_link_libraries( rawwifi ${PCAP_LIBRARY} ${NL_LIBRARIES} )
