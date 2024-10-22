if(NOT DEFINED MACRO_SUBMODULE_LOADED)
	set( ADDED_MODULES "" CACHE INTERNAL "" )
	set( MACRO_SUBMODULE_LOADED TRUE )
endif()


macro(add_submodule module_name)
	get_property( added_modules CACHE ADDED_MODULES PROPERTY VALUE )
	list( FIND added_modules "${module_name}" module_index )
	if ( module_index EQUAL -1 )
		message(STATUS "Adding module : ${module_name}")
		add_subdirectory( ${CMAKE_SOURCE_DIR}/../lib/${module_name} ${CMAKE_BINARY_DIR}/${module_name} )
		get_property( added_modules CACHE ADDED_MODULES PROPERTY VALUE )
		list( APPEND added_modules "${module_name}" )
		set( ADDED_MODULES "${added_modules}" CACHE INTERNAL "" )
		get_property( added_modules_new CACHE ADDED_MODULES PROPERTY VALUE )
	endif()
endmacro()
