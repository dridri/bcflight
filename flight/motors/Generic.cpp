#include <Debug.h>
#include "Generic.h"

Generic::Generic( Servo* servo, float minspeed, float maxSpeed )
	: Motor()
	, mServo( servo )
	, mMinSpeed( std::min( std::max( minspeed, 0.0f ), 1.0f ) )
	, mMaxSpeed( std::min( std::max( maxSpeed, 0.0f ), 1.0f ) )
{
}


Generic::~Generic()
{
}


void Generic::setSpeedRaw( float speed, bool force_hw_update )
{
	speed = std::max( mMinSpeed, std::min( mMaxSpeed, speed ) );
	mServo->setValue( speed, force_hw_update );
/*
	speed = std::max( 0, std::min( (int)( 255.0f * mMaxSpeed ), speed ) );
	mServo->setValue( 50 + speed * 200 / 256 );
*/
}


void Generic::Disarm()
{
	mServo->Disarm();
}
