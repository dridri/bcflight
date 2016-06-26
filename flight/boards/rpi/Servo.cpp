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

#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Servo.h"
#include "Debug.h"
#include "pi-blaster.h"

Servo::Servo( int pin, int us_min, int us_max )
	: mID( pin )
	, mIdle( (int)( us_min * 0.8f ) )
	, mMin( us_min )
	, mMax( us_max )
	, mRange( us_max - us_min )
{
}


Servo::~Servo()
{
}

/*
void Servo::setValue( int width_ms )
{
	char buf[32] = "";

	if ( mID >= 0 and mFD >= 0 ) {
		sprintf( buf, "%d=%d\n", mID, width_ms );
		write( mFD, buf, strlen(buf) );
	}
}
*/

void Servo::setValue( float p, bool force_update )
{
	if ( p < 0.0f ) {
		p = 0.0f;
	}
	if ( p > 1.0f ) {
		p = 1.0f;
	}

	uint32_t us = mMin + (uint32_t)( mRange * p );

// 	PiBlasterSetPWM( mID, p );
	PiBlasterSetPWMus( mID, us );
	if ( force_update ) {
		PiBlasterUpdatePWM();
	}
}


void Servo::HardwareSync()
{
	PiBlasterUpdatePWM();
}


void Servo::Disarm()
{
	PiBlasterSetPWMus( mID, mIdle );
	PiBlasterUpdatePWM();
}


void Servo::Disable()
{
	PiBlasterSetPWMus( mID, 0 );
	PiBlasterUpdatePWM();
}
