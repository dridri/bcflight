# - Find libiw
#
# This module defines
#  LIBIW_FOUND - whether the libiw library was found
#  LIBIW_LIBRARIES - the libiw library
#  LIBIW_INCLUDE_DIR - the include path of the libiw library

execute_process( COMMAND sh -c "${CMAKE_C_COMPILER} -print-search-dirs | grep libraries | cut -d'=' -f2 | tr : ';'" OUTPUT_VARIABLE ARM_LIBS )
find_library (LIBIW_LIBRARY iw PATHS ${ARM_LIBS} )

if ( LIBIW_LIBRARY )
    set(LIBIW_LIBRARIES  ${LIBIW_LIBRARY})
endif ( LIBIW_LIBRARY )

find_path (LIBIW_INCLUDE_DIR NAMES iwlib.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libiw  DEFAULT_MSG  LIBIW_LIBRARY LIBIW_INCLUDE_DIR)

mark_as_advanced(LIBIW_INCLUDE_DIR LIBIW_LIBRARY)
