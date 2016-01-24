#include <cmath>
#include "Motor.h"

Motor::Motor()
	: mSpeed( -1 )
{
}


Motor::~Motor()
{
}


const float Motor::speed() const
{
	return mSpeed;
}


void Motor::KeepSpeed()
{
	setSpeed( mSpeed );
}


void Motor::setSpeed( float speed, bool force_hw_update )
{
	if ( speed == mSpeed and not force_hw_update ) {
		return;
	}
/*
	uint8_t uspeed = (int)( fmaxf( 0, fminf( 255, (int)( speed * 255.0f ) ) ) );
	setSpeedRaw( uspeed );
*/
	setSpeedRaw( speed, force_hw_update );
	mSpeed = speed;
}
