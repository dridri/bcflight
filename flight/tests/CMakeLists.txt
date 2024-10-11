add_executable(
	flight_tests
	main.cpp
	stabilizer/test_BiquadFilter.cpp
)

target_link_libraries(flight_tests PRIVATE -L../ controllerbase luacore libdebug ${LIBS} ${BOARD_LIBS} Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(flight_tests)

