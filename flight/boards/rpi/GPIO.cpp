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

#include <wiringPi.h>
#include <softPwm.h>
#include <Debug.h>
#include "GPIO.h"


void GPIO::setMode( int pin, GPIO::Mode mode )
{
	if ( mode == Output ) {
		pinMode( pin, OUTPUT );
	} else {
		pinMode( pin, INPUT );
	}
}


void GPIO::setPWM( int pin, int initialValue, int pwmRange )
{
	setMode( pin, Output );
	softPwmCreate( pin, initialValue, pwmRange );
}


void GPIO::Write( int pin, bool en )
{
	digitalWrite( pin, en );
}


bool GPIO::Read( int pin )
{
	return digitalRead( pin );
}


void GPIO::SetupInterrupt( int pin, GPIO::ISRMode mode )
{
	char command[256] = "";
	std::string smode = "falling";

	if ( mode == GPIO::Rising ) {
		smode = "rising";
	} else if ( mode == GPIO::Both ) {
		smode = "both";
	}

	sprintf( command, "gpio edge %d %s", pin, smode.c_str() );
	system( command );
}


int GPIO::WaitForInterrupt( int pin, int timeout_ms )
{
	return waitForInterrupt( pin, timeout_ms );
}
