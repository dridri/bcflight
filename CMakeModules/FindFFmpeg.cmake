include(FindPackageHandleStandardArgs)

set( FFMPEG_LIB "${CMAKE_FIND_ROOT_PATH}/arm-linux-gnueabihf/lib" )
set( FFMPEG_INC "${CMAKE_FIND_ROOT_PATH}/arm-linux-gnueabihf/include" )


foreach (_component ${FFmpeg_FIND_COMPONENTS})
	string( TOLOWER ${_component} libname )
	find_path( FFmpeg_${_component}_INCLUDE_DIR
		NAMES
			lib${libname}/${libname}.h
		PATHS
			"${FFMPEG_INC}"
		PATH_SUFFIXES
			include
	)
	find_library( FFmpeg_${_component}_LIBRARIES
		NAMES
			${libname}
		PATHS
			"${FFMPEG_LIB}"
		PATH_SUFFIXES
			lib
		HINTS
			${FFmpeg_${_component}_INCLUDE_DIR}/../lib
			${FFmpeg_${_component}_INCLUDE_DIR}/../lib/arm-linux-gnueabihf
	)
	if ( FFmpeg_${_component}_LIBRARIES )
		set( FFMPEG_${_component}_FOUND 1 )
	endif()
	set( FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFmpeg_${_component}_LIBRARIES})
	list( APPEND FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS} ${FFmpeg_${_component}_INCLUDE_DIR})
endforeach ()

if ( FFMPEG_INCLUDE_DIRS )
	list( REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS )
endif ()

if ( FFMPEG_LIBRARIES AND FFMPEG_INCLUDE_DIRS )
	set( FFmpeg_FOUND 1 )
endif()

mark_as_advanced( FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES )
