include(FindPackageHandleStandardArgs)

set( LIBGPS_LIB "${CMAKE_FIND_ROOT_PATH}/aarch64-linux-gnu/lib" )
set( LIBGPS_INC "${CMAKE_FIND_ROOT_PATH}/aarch64-linux-gnu/include" )


find_library( GPS_LIBRARIES
	NAMES
		gps
	PATHS
		"${LIBGPS_LIB}"
	PATH_SUFFIXES
		lib
)

find_path( GPS_INCLUDE_DIR
	NAMES
		gps.h
	PATHS
		"${LIBGPS_INC}"
	PATH_SUFFIXES
		include
)

if ( GPS_LIBRARIES )
	set( GPS_FOUND 1 )
endif()

mark_as_advanced( GPS_INCLUDE_DIR GPS_LIBRARIES )
