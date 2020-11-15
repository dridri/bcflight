set( BOARD_LIBS -lpthread -ldl )

# see boards/rpi/board.cmake for further examples


set( TARGET_LINUX 1 )
set( TARGET_CPU_BITS 32 )


if ( ${BUILD_sensors} MATCHES 1 )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/sensors/fake_sensors/FakeAccelerometer.cpp )
	set( BOARD_SOURCES ${BOARD_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/sensors/fake_sensors/FakeGyroscope.cpp )
endif()

function( board_strip )
	add_custom_target( flight ALL
						COMMAND strip -s -o flight flight_unstripped
						DEPENDS flight_unstripped
						COMMENT "Stripping executable"
						VERBATIM
	)
endfunction()
