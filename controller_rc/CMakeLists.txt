cmake_minimum_required( VERSION 2.6 )

# Command line options
SET( board "rpi" CACHE BOOL "board" )
SET( rawwifi "0" CACHE BOOL "rawwifi" )
option( board "board" )
option( rawwifi "rawwifi" )

if ( NOT ${board} MATCHES OFF AND NOT ${board} MATCHES "generic" )
	set( CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../Toolchain-${board}.cmake )
endif()

project( controller_rc )

# Set compiler options
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -O0 -D_DEFAULT_SOURCE" )
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
include_directories( ${CMAKE_SOURCE_DIR} )
include_directories( ${CMAKE_BINARY_DIR} )
add_definitions( -DBOARD="${board}" -DBOARD_${board} )
if ( ${USE_MINISTD} MATCHES TRUE )
	add_definitions( -DSTD=ministd )
else()
	add_definitions( -DSTD=std )
endif()

include_directories( ${CMAKE_SOURCE_DIR}/../libdebug )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller/links )
if ( ${rawwifi} MATCHES "1" OR ${rawwifi} MATCHES "yes" OR ${rawwifi} MATCHES "true" )
	include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )
	set( RAWWIFI_LIBS rawwifi -lpcap )
	add_definitions( -DBUILD_rawwifi )
endif()
include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )
add_subdirectory( ${CMAKE_SOURCE_DIR}/../libcontroller libcontroller )
add_subdirectory( ${CMAKE_SOURCE_DIR}/../libdebug libdebug )

add_custom_target( libluajit_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_luajit.sh ${CMAKE_BINARY_DIR} ${CMAKE_C_COMPILER} ${CMAKE_ASM_COMPILER} ${CROSS}strip "-m32" )
add_custom_target( ui_headers COMMAND
	${UIC} ${CMAKE_SOURCE_DIR}/ui/main.ui > ${CMAKE_BINARY_DIR}/ui_main.h
	&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/window.ui > ${CMAKE_BINARY_DIR}/ui_window.h
	&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/calibrate.ui > ${CMAKE_BINARY_DIR}/ui_calibrate.h
	&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/camera.ui > ${CMAKE_BINARY_DIR}/ui_camera.h
	&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/network.ui > ${CMAKE_BINARY_DIR}/ui_network.h
	&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/settings.ui > ${CMAKE_BINARY_DIR}/ui_settings.h
)

# add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/rc_data.cpp COMMAND ${RCC} ${CMAKE_SOURCE_DIR}/ui/data.qrc > ${CMAKE_BINARY_DIR}/rc_data.cpp WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )
add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/MainWindow.moc.cpp COMMAND ${MOC} ${CMAKE_SOURCE_DIR}/ui/MainWindow.h > ${CMAKE_BINARY_DIR}/MainWindow.moc.cpp WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )

file( GLOB_RECURSE BOARD_SOURCES boards/${board}/*.cpp boards/${board}/*.c )
file( GLOB_RECURSE ADCS_SOURCES ADCs/*.cpp ADCs/*.c )

add_executable( controller_rc
				main.cpp
				Config.cpp
				ui/GlobalUI.cpp
				ui/MainWindow.cpp
				${CMAKE_BINARY_DIR}/rc_data.cpp
				${CMAKE_BINARY_DIR}/MainWindow.moc.cpp
				${ADCS_SOURCES}
				${BOARD_SOURCES}
				)

add_dependencies( controller_rc controller )
add_dependencies( controller_rc libluajit_static )
add_dependencies( controller_rc ui_headers )


target_link_libraries( controller_rc -L./ controller libdebug libluajit_static.a -lrt -ldl ${RAWWIFI_LIBS} -lz pthread ${BOARD_LIBS} )

install( TARGETS controller_rc RUNTIME DESTINATION bin )
