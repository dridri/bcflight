cmake_minimum_required( VERSION 2.6 )
project( joystick )

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb3 -Wall -Wuninitialized -std=gnu11 -fgnu89-inline" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3 -Wall -Wuninitialized -std=gnu++11 -Wno-pmf-conversions -Wno-unused-result -Wno-unused" )
set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -ggdb3 -Wall -std=gnu11" )
set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -ggdb3 -Wall -std=gnu11" )

add_executable( joystick main.cpp Socket.cpp )
target_link_libraries( joystick -ludev )

install( TARGETS joystick RUNTIME DESTINATION bin )
