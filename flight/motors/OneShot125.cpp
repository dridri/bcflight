#include <cmath>
#include "OneShot125.h"

#define SCALE 0.5
// #define SCALE 1


OneShot125::OneShot125( uint32_t pin, int us_min, int us_max )
	: Motor()
	, mPWM( new PWM( pin, 1000000, 1000, 1 ) )
// 	, mPWM( new PWM( pin, 2000000, 1000, 1 ) )
	, mPin( pin )
	, mMinUS( us_min )
	, mMaxUS( us_max )
{
}


OneShot125::~OneShot125()
{
}


void OneShot125::setSpeedRaw( float speed, bool force_hw_update )
{
	if ( std::isnan( speed ) or std::isinf( speed ) ) {
		return;
	}
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

	uint32_t us = mMinUS + (uint32_t)( (float)( mMaxUS - mMinUS ) * speed );
	mPWM->SetPWMus( us * SCALE );

	if ( force_hw_update ) {
		mPWM->Update();
	}
}


void OneShot125::Disarm()
{
	mPWM->SetPWMus( 100 * SCALE );
	mPWM->Update();
}


void OneShot125::Disable()
{
	if ( not mPWM ) {
		return;
	}

	mPWM->SetPWMus( 0 );
	mPWM->Update();
}
