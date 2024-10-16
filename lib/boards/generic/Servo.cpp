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
#include <stdint.h>
#include "Servo.h"

Servo::Servo( int pin, int us_min, int us_max )
	: mID( pin )
	, mMin( us_min )
	, mMax( us_max )
	, mRange( us_max - us_min )
{
}


Servo::~Servo()
{
}


void Servo::setValue( float p, bool force_update )
{
	if ( p < 0.0f ) {
		p = 0.0f;
	}
	if ( p > 1.0f ) {
		p = 1.0f;
	}
	uint32_t us = mMin + (uint32_t)( mRange * p );

	// Set PWM here. If 'force_update' indicates that the PWM should be updated now
	// (consistent only when the hardware/software implementation need an explicit call to update PWM)
}


void Servo::HardwareSync()
{
	// ^ consistent only when the hardware/software implementation need an explicit call to update PWM
}


void Servo::Disarm()
{
	// After calling this function, PWM should output a value below the minimum arming pulse width
	// This ensure that the motors will not start spinning at any time, letting user to touch them without risks
}


void Servo::Disable()
{
	// After calling this function, PWM should not output anything and setting its pin to a 0V output
	// This is used to calibrate and configure ESCs
}