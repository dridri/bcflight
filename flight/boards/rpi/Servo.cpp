#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Servo.h"
#include "Debug.h"
#include "pi-blaster.h"

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
	PiBlasterSetPWMus( mID, 0 );
	PiBlasterUpdatePWM();
}
