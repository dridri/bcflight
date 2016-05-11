cmake_minimum_required( VERSION 2.6 )
project( rawwifi )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3" )

add_library(rawwifi
			rx.c
			tx.c
			rawwifi.c
			radiotap.c
			)

target_link_libraries( rawwifi -lrt )
