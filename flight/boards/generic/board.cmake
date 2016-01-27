set( BOARD_LIBS -lpthread -ldl )

# see boards/rpi/board.cmake for further examples



function( board_strip )
	add_custom_target( flight ALL
						COMMAND strip -s -o flight flight_unstripped
						DEPENDS flight_unstripped
						COMMENT "Stripping executable"
						VERBATIM
	)
endfunction()
