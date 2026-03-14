set( BOARD_LIBS -lpthread -ldl )

# see boards/rpi/board.cmake for further examples


set( TARGET_LINUX 1 CACHE INTERNAL "" )
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set( TARGET_CPU_BITS 64 CACHE INTERNAL "" )
else()
	set( TARGET_CPU_BITS 32 CACHE INTERNAL "" )
endif()

find_package( PkgConfig REQUIRED )

if ( ${BUILD_video} MATCHES 1 )
	if ( ${TARGET_LINUX} MATCHES "1" )
		pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
		message( STATUS "Found libdrm : ${LIBDRM_INCLUDE_DIRS} ${LIBDRM_LIBRARIES}" )
		include_directories( ${LIBDRM_INCLUDE_DIRS} )

		pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
		message( STATUS "Found libcamera : ${LIBCAMERA_LIBRARIES}" )
		include_directories(${LIBCAMERA_INCLUDE_DIRS})
		set( BOARD_LIBS ${BOARD_LIBS} ${LIBCAMERA_LIBRARIES} )
	endif()
endif()

if ( BUILD_audio )
	set( BOARD_LIBS ${BOARD_LIBS} -lasound )
endif()


set( BOARD_LIBS ${BOARD_LIBS} -lGLESv2 -lEGL )

pkg_check_modules( LIBGBM gbm )
if ( LIBGBM_FOUND )
	message( STATUS "GLContext: DRM/GBM backend enabled" )
	add_definitions( -DHAVE_DRM )
	set( BOARD_LIBS ${BOARD_LIBS} -ldrm -lgbm )
	include_directories( ${LIBGBM_INCLUDE_DIRS} )
endif()

pkg_check_modules( LIBX11 x11 )
if ( LIBX11_FOUND )
	message( STATUS "GLContext: X11 backend enabled" )
	add_definitions( -DHAVE_X11 )
	set( BOARD_LIBS ${BOARD_LIBS} ${LIBX11_LIBRARIES} )
	include_directories( ${LIBX11_INCLUDE_DIRS} )
endif()
set( BOARD_LIBS ${BOARD_LIBS} CACHE INTERNAL "" )


function( board_strip )
	add_custom_target( flight ALL
						COMMAND strip -s -o flight flight_unstripped
						DEPENDS flight_unstripped
						COMMENT "Stripping executable"
						VERBATIM
	)
endfunction()
