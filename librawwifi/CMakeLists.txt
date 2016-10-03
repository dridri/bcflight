cmake_minimum_required( VERSION 2.6 )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
project( rawwifi )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -std=gnu11" )

# find_package( Libiw REQUIRED )
find_package( LibPCAP REQUIRED )
find_package( LibNL REQUIRED )

include_directories( ${NL_INCLUDE_DIR} )

add_library(rawwifi
			interface.c
			rx_base.c
			rx_weighted.c
			tx.c
			rawwifi.c
			radiotap.c
			blocks.c
			)

target_link_libraries( rawwifi ${NL_LIBRARIES} -lrt )
