cmake_minimum_required( VERSION 2.6 )

# Command line options
SET( board "rpi" CACHE BOOL "board" )
option( board "board" )

if ( NOT ${board} MATCHES OFF AND NOT ${board} MATCHES "generic" )
	set( CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../Toolchain-${board}.cmake )
endif()

project( controller_rc )

# Set compiler options
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -O0 -D_GNU_SOURCE -D_BSD_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb3 -O0 -std=c++11 -DGE_LINUX -DGE_EGL -fpermissive -Wno-pmf-conversions" )

# Check chosen board
macro( DIRLIST result curdir removes )
	file(GLOB children RELATIVE ${curdir} ${curdir}/*)
	set(dirlist "")
	foreach(child ${children})
		if ( NOT "${child}" MATCHES ".md" )
			string(REPLACE ".cpp" "" child ${child})
			string(REPLACE ".h" "" child ${child})
			list(APPEND dirlist ${child})
		endif()
	endforeach()
	if ( NOT ${removes} MATCHES OFF )
		list(REMOVE_ITEM dirlist ${removes})
	endif()
	list(REMOVE_DUPLICATES dirlist)
	set(${result} ${dirlist})
endmacro()
DIRLIST( BOARDS ${CMAKE_SOURCE_DIR}/boards "" )

list( FIND BOARDS ${board} board_found )
if ( ${board_found} LESS 0 )
	message( FATAL_ERROR "Board ${board} not supported.\nAvailable boards are :\n${BOARDS}" )
endif()

include( boards/${board}/board.cmake )
include_directories( boards/${board} )
include_directories( ${CMAKE_BINARY_DIR} )
add_definitions( -DBOARD="${board}" -DBOARD_${board} )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../libcontroller libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller/links )
include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )
include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )

add_custom_target( libluajit_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_luajit.sh ${CMAKE_BINARY_DIR} ${CMAKE_C_COMPILER} ${CMAKE_ASM_COMPILER} ${CROSS}strip "-m32" )
add_custom_target( ui_headers COMMAND ${UIC} ${CMAKE_SOURCE_DIR}/ui/main.ui > ${CMAKE_BINARY_DIR}/ui_main.h )
add_custom_target( rcc_datas COMMAND ${RCC} ${CMAKE_SOURCE_DIR}/ui/data.qrc> ${CMAKE_BINARY_DIR}/rc_data.cpp )

file( GLOB_RECURSE BOARD_SOURCES boards/${board}/*.cpp boards/${board}/*.c )
file( GLOB_RECURSE ADCS_SOURCES ADCs/*.cpp ADCs/*.c )

add_executable( controller_rc
				main.cpp
				Config.cpp
				Stream.cpp
				RendererHUD.cpp
				RendererHUDNeo.cpp
				DecodedImage.cpp
				Matrix.cpp
				ui/GlobalUI.cpp
				ui/MainWindow.cpp
				${CMAKE_BINARY_DIR}/rc_data.cpp
				${ADCS_SOURCES}
				${BOARD_SOURCES}
				)

add_dependencies( controller_rc controller )
add_dependencies( controller_rc libluajit_static )
add_dependencies( controller_rc ui_headers )
add_dependencies( controller_rc rcc_datas )

target_link_libraries( controller_rc -L./ controller libluajit_static.a rawwifi -lrt -ldl -lpcap -lz pthread ${BOARD_LIBS} )

install( TARGETS controller_rc RUNTIME DESTINATION bin )