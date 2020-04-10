/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <string>
#include "Main.h"
#include "Debug.h"
#include "Thread.h"

extern "C" void board_init();
extern "C" void board_printf( int level, const char* s, ... );


int main( int ac, char** av )
{
	board_init();
	board_printf( 0, "Board '%s' initialized\n", BOARD );


	// Execute Main entry functionn which will spawn threads, then return back here
	Main::flight_entry( ac, av );

	// Start infinite pseudo-threadmanager loop
	Thread::ThreadEntry();

	// Should never happen
	while ( 1 ) {
	}
	return 0;
}
