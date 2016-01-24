#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
	// After calling this function, PWM should not output anything and setting its pin to a 0V output
	// This ensure that the motors will not receive any data, letting user to touch them without risks
}