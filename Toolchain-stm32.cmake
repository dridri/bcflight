SET( CMAKE_SYSTEM_NAME Generic )
SET( CMAKE_SYSTEM_VERSION 1 )


SET( CROSS arm-none-eabi- )
SET( CMAKE_C_COMPILER "arm-none-eabi-gcc" )
SET( CMAKE_CXX_COMPILER "arm-none-eabi-g++" )
SET( CMAKE_ASM_COMPILER "arm-none-eabi-gcc" )
set( CMAKE_AR "arm-none-eabi-ar" CACHE PATH "" FORCE )
set( CMAKE_RANLIB "arm-none-eabi-ranlib" CACHE PATH "" FORCE )
set( CMAKE_LINKER "arm-none-eabi-g++" CACHE PATH "" FORCE )
# set( CMAKE_LINKER "arm-none-eabi-gcc" CACHE PATH "" FORCE )
# set( CMAKE_CXX_LINK_EXECUTABLE "arm-none-eabi-gcc <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" )
set( CMAKE_SIZE "arm-none-eabi-size" )
set( CMAKE_OBJCOPY "arm-none-eabi-objcopy" )

set( CMAKE_C_FLAGS "-fdata-sections -ffunction-sections -nostartfiles" )
set( CMAKE_CXX_FLAGS "-fdata-sections -ffunction-sections -nostartfiles -fno-use-cxa-atexit" )
set( CMAKE_ASM_FLAGS "-fdata-sections -ffunction-sections -nostartfiles" )

SET( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
SET( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
SET( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

SET( CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "" )
SET( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" )
