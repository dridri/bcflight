find_path(SHINE_INCLUDE_DIR
  NAMES shine/layer3.h)

find_library(SHINE_LIBRARY
  NAMES shine)

set(SHINE_LIBRARIES ${SHINE_LIBRARY})
set(SHINE_INCLUDE_DIRS ${SHINE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set SHINE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(SHINE DEFAULT_MSG SHINE_LIBRARY SHINE_INCLUDE_DIR)

