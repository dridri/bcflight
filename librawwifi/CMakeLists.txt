cmake_minimum_required( VERSION 2.6 )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
project( rawwifi )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -std=gnu11" )

find_package( Libiw REQUIRED )
find_package( LibPCAP REQUIRED )

add_library(rawwifi
			rx.c
			tx.c
			rawwifi.c
			radiotap.c
			blocks.c
			)

target_link_libraries( rawwifi -lrt )
