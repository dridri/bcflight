set( TARGET_LINUX 0 )
set( TARGET_CPU_BITS 32 )


if ( ${variant} MATCHES OFF OR "${variant}" STREQUAL "" )
# 	- STM32F10x
# 	- STM32F2xx
# 	- STM32F30x
	message( STATUS "Please set 'variant', supported variants are :
	- STM32F405xx
	- STM32F415xx
	- STM32F407xx
	- STM32F417xx
	- STM32F427xx
	- STM32F437xx
	- STM32F429xx
	- STM32F439xx
	- STM32F401xC
	- STM32F401xE
	- STM32F411xE
	- STM32F413xH
	- STM32F446xx
	")
	message( FATAL_ERROR )
endif()

string( SUBSTRING "${variant}" 0 7 variant_group )
string( TOLOWER "${variant_group}" variant_group )
message("${variant_group}")

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mlittle-endian -mthumb -mthumb-interwork -mcpu=cortex-m4 -fsingle-precision-constant" )
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -fpermissive" )
# set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -mfloat-abi=soft -mcpu=cortex-m4 -mlittle-endian -mthumb -mthumb-interwork -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/arm-gcc-link.ld")
set(CMAKE_C_FLAGS "-ffast-math -fno-math-errno -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mlittle-endian -mthumb -mthumb-interwork -mcpu=cortex-m4")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=32 -U_FORTIFY_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fpermissive -fno-exceptions -fno-non-call-exceptions -fno-use-cxa-atexit" )
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -specs=nano.specs -Wl,--build-id" )
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -D_NO_RTTI" )
set(CMAKE_LD_FLAGS "-ffast-math -fno-math-errno -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mcpu=cortex-m4 -mlittle-endian -mthumb -mthumb-interwork -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/arm-gcc-link.ld")
set(CMAKE_LINKER_FLAGS "${CMAKE_LD_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_LD_FLAGS}")

set( BOARD_LIBS -lm )

# Enable usage of libministd to reduce memory usage
add_subdirectory( ${CMAKE_SOURCE_DIR}/../libministd ${CMAKE_BINARY_DIR}/libministd )
include_directories( ${CMAKE_SOURCE_DIR}/../libministd/include )
set( BOARD_DEPENDENCIES "${BOARD_DEPENDENCIES} ministd" )
set( USE_MINISTD TRUE )

if ( ${variant_group} MATCHES "stm32f4" )
	add_definitions( -D__FPU_PRESENT=1 -D__FPU_USED=1 )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/init_${variant_group}.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/startup_stm32f40_41xxx.s )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/system_stm32f4xx.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_cortex.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_rcc.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_gpio.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_uart.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_usart.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_dma.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_spi.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_i2c.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_tim.c )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_hal_tim_ex.c )
# 	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_rcc.c )
# 	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_gpio.c )
# 	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/stm32f4xx_usart.c )
# 	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/misc.c )
# 	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/tm_stm32f4_gpio.c )
# 	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/${variant_group}/tm_stm32f4_usart.c )
endif()
message( "src : ${BOARD_SOURCES}")
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/cmsis )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/boards/stm32/cmsis_boot )
include_directories( ./${variant_group} )
add_definitions( -D${variant} -DBOARD_VARIANT="${variant}" )
add_definitions( -DVARIANT_GROUP="${variant_group}" -DVARIANT_GROUP_${variant_group} )
add_definitions( -DBOARD_VARIANT_FILE="${variant_group}/${variant_group}xx_hal.h" )

message( STATUS "Variant group : ${variant_group}" )
message( STATUS "Variant file : ${variant_group}/${variant_group}xx.h" )


function( board_strip )
	add_custom_target( flight_stripped ALL
						COMMAND arm-none-eabi-strip -s -o flight_stripped flight_unstripped
						DEPENDS flight_unstripped
						COMMENT "Stripping executable"
						VERBATIM
	)
	add_custom_target( flight ALL
						COMMAND arm-none-eabi-objcopy -O binary flight_stripped flight
						DEPENDS flight_stripped
						COMMENT "Converting executable to BIN file 'flight'"
						VERBATIM
	)
endfunction()
