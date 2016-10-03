# - Find libnl-3
#
# This module defines
#  NL_LIBRARIES - the libnl library
#  NL_INCLUDE_DIR - the include path of the libnl library

execute_process( COMMAND sh -c "${CMAKE_C_COMPILER} -print-search-dirs | grep libraries | cut -d'=' -f2 | tr : ';'" OUTPUT_VARIABLE ARM_LIBS )
find_library (NL_LIBRARY nl-3 PATHS ${ARM_LIBS} )
find_library (NLGENL_LIBRARY nl-genl-3 PATHS ${ARM_LIBS} )
find_library (NLROUTE_LIBRARY nl-route-3 PATHS ${ARM_LIBS} )

set(NL_LIBRARIES ${NLGENL_LIBRARY} ${NLROUTE_LIBRARY} ${NL_LIBRARY} )

find_path( NL_INCLUDE_DIR PATH_SUFFIXES include/libnl3 NAMES netlink/version.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libnl-3  DEFAULT_MSG  NL_LIBRARY NL_INCLUDE_DIR)

mark_as_advanced(NL_INCLUDE_DIR NL_LIBRARY)
