cmake_minimum_required( VERSION 3.5 )
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../CMakeModules" )
include( add_submodule )

set( CMAKE_CXX_STANDARD 17 )
set( UIC uic )
set( RCC rcc )
set( MOC moc )

# Command line options
SET( rawwifi "0" CACHE BOOL "rawwifi" )
option( rawwifi "rawwifi" )

if ( NOT ${board} MATCHES OFF AND NOT ${board} MATCHES "generic" )
	set( CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../Toolchain-${board}.cmake )
endif()

enable_language( C CXX ASM )
project( controller_rc )

add_definitions( -DSYSTEM_NAME=${CMAKE_SYSTEM_NAME} -DSYSTEM_NAME_${CMAKE_SYSTEM_NAME} )
add_definitions( -DBOARD="${board}" -DBOARD_${board} -DVARIANT="${variant}" -DVARIANT_${variant} )
add_definitions( -DVERSION_STRING="bcflight_${board}_${VERSION_MAJOR}.${VERSION_MINOR}_${GIT_HASH}" )
add_definitions( -DTARGET_CPU_BITS=${TARGET_CPU_BITS} )
add_submodule( boards )

# Set compiler options
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ggdb3 -O0 -D_DEFAULT_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb3 -O0 -std=c++17 -DGE_LINUX -DGE_EGL -fpermissive -Wno-pmf-conversions -Wno-psabi" )

include_directories( ${CMAKE_SOURCE_DIR}/../lib/debug )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/controller )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/links )
if ( ${rawwifi} MATCHES "1" OR ${rawwifi} MATCHES "yes" OR ${rawwifi} MATCHES "true" )
	include_directories( ${CMAKE_SOURCE_DIR}/../lib/rawwifi )
	# set( RAWWIFI_LIBS rawwifi -lpcap )
	add_definitions( -DBUILD_rawwifi )
endif()

include_directories( ${CMAKE_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/boards/${board} )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/luacore/src )
include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/boards/${board} )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/boards/common )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/maths )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/sensors )
add_submodule( controller )
add_submodule( sensors )
add_submodule( debug )


file( GLOB HEADERS
	${CMAKE_SOURCE_DIR}/*.h
	${CMAKE_SOURCE_DIR}/boards/rpi/*.h
	${CMAKE_SOURCE_DIR}/../lib/controller/*.h
	${CMAKE_SOURCE_DIR}/../lib/links/*.h
)
file( GLOB BOARD_HEADERS ${CMAKE_SOURCE_DIR}/../lib/boards/common/*.h ${CMAKE_SOURCE_DIR}/../lib/boards/${board}/*.h )
list( APPEND HEADERS ${BOARD_HEADERS} )
message( WARNING ${HEADERS} )

set_source_files_properties( ${CMAKE_BINARY_DIR}/lua_init.cpp PROPERTIES GENERATED TRUE )
set_source_files_properties( ${CMAKE_BINARY_DIR}/rc_data.cpp PROPERTIES GENERATED TRUE )


add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/lua_init.cpp
	DEPENDS ${CMAKE_SOURCE_DIR}/../lib/luacore/classes.auto.lua ${HEADERS}
	COMMAND ${CMAKE_SOURCE_DIR}/../lib/luacore/classes.auto.lua --init=lua_init --output="${CMAKE_BINARY_DIR}/lua_init.cpp" ${HEADERS}
)
add_custom_command(
	OUTPUT
		${CMAKE_BINARY_DIR}/ui_main.h
		${CMAKE_BINARY_DIR}/ui_window.h
		${CMAKE_BINARY_DIR}/ui_calibrate.h
		${CMAKE_BINARY_DIR}/ui_camera.h
		${CMAKE_BINARY_DIR}/ui_network.h
		${CMAKE_BINARY_DIR}/ui_settings.h
	COMMAND
		${UIC} ${CMAKE_SOURCE_DIR}/ui/main.ui > ${CMAKE_BINARY_DIR}/ui_main.h
		&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/window.ui > ${CMAKE_BINARY_DIR}/ui_window.h
		&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/calibrate.ui > ${CMAKE_BINARY_DIR}/ui_calibrate.h
		&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/camera.ui > ${CMAKE_BINARY_DIR}/ui_camera.h
		&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/network.ui > ${CMAKE_BINARY_DIR}/ui_network.h
		&& ${UIC} ${CMAKE_SOURCE_DIR}/ui/settings.ui > ${CMAKE_BINARY_DIR}/ui_settings.h
	DEPENDS
		${CMAKE_SOURCE_DIR}/ui/main.ui
		${CMAKE_SOURCE_DIR}/ui/window.ui
		${CMAKE_SOURCE_DIR}/ui/calibrate.ui
		${CMAKE_SOURCE_DIR}/ui/camera.ui
		${CMAKE_SOURCE_DIR}/ui/network.ui
		${CMAKE_SOURCE_DIR}/ui/settings.ui
)
add_custom_target(ui_headers
	DEPENDS
		${CMAKE_BINARY_DIR}/ui_main.h
		${CMAKE_BINARY_DIR}/ui_window.h
		${CMAKE_BINARY_DIR}/ui_calibrate.h
		${CMAKE_BINARY_DIR}/ui_camera.h
		${CMAKE_BINARY_DIR}/ui_network.h
		${CMAKE_BINARY_DIR}/ui_settings.h
)

add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/rc_data.cpp COMMAND ${RCC} ${CMAKE_SOURCE_DIR}/ui/data.qrc > ${CMAKE_BINARY_DIR}/rc_data.cpp WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )
add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/MainWindow.moc.cpp COMMAND ${MOC} ${CMAKE_SOURCE_DIR}/ui/MainWindow.h > ${CMAKE_BINARY_DIR}/MainWindow.moc.cpp WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )

add_executable( controller_rc
	main.cpp
	Config.cpp
	ui/GlobalUI.cpp
	ui/MainWindow.cpp
	boards/${board}/ControllerClient.cpp
	${CMAKE_BINARY_DIR}/rc_data.cpp
	${CMAKE_BINARY_DIR}/MainWindow.moc.cpp
	${CMAKE_BINARY_DIR}/lua_init.cpp
)

add_dependencies( controller_rc controller )
add_dependencies( controller_rc sensors )
add_dependencies( controller_rc maths )
add_dependencies( controller_rc ui_headers )
if ( BOARD_DEPENDENCIES )
	add_dependencies( controller_rc ${BOARD_DEPENDENCIES} )
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -L${CMAKE_BINARY_DIR} ${BOARD_LIBS_PATHS}" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -L${CMAKE_BINARY_DIR} ${BOARD_LIBS_PATHS}" )
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -L${CMAKE_BINARY_DIR} ${BOARD_LIBS_PATHS}" )
set(CMAKE_LINKER_FLAGS "${CMAKE_LD_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -L${CMAKE_BINARY_DIR} ${BOARD_LIBS_PATHS}" )
target_link_libraries(
	controller_rc
	-L./
	controller
	sensors
	debuglib
	links
	boards
	maths
	${RAWWIFI_LIBS}
	Qt5Widgets Qt5Gui Qt5PrintSupport Qt5Core
	-lrt
	-ldl
	-lz
	${BOARD_LIBS}
	pthread
)
# set( CONTROLLER_BOARD_LIBS ${CONTROLLER_BOARD_LIBS}  -lGLESv2 -lEGL )

install( TARGETS controller_rc RUNTIME DESTINATION bin )
