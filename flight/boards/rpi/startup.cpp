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

#include <Main.h>


int main( int ac, char** av )
{
	Debug::setDebugLevel( Debug::Verbose );
	int ret = Main::flight_entry( ac, av );

	if ( ret == 0 ) {
		while ( 1 ) {
			usleep( 1000 * 1000 * 100 );
		}
	}

	return 0;
}
