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

// #include <peripherals/SmartAudio.h>
// #include <pigpio.h>


int main( int ac, char** av )
{
	Debug::setDebugLevel( Debug::Verbose );
	// gpioInitialise();
	// Debug::setDebugLevel( Debug::Trace );
	// Serial* s = new Serial( "/dev/ttyAMA1", 4800, 100 );
	// SmartAudio* sa = new SmartAudio( s, 12 );
	// sa->Update();
	// sa->setPower( 0 );
	// sa->setFrequency( 5645 );
	// sa->setChannel( 35 );
	// exit(0);

	int ret = Main::flight_entry( ac, av );

	if ( ret == 0 ) {
		while ( 1 ) {
			usleep( 1000 * 1000 * 100 );
		}
	}

	return 0;
}
