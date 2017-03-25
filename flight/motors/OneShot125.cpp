#include "OneShot125.h"


OneShot125::OneShot125( uint32_t pin, int us_min, int us_max )
	: Motor()
	, mPWM( new PWM( pin, 6400000, 800, 2, PWM::MODE_PWM, true ) )
	, mMinUS( us_min )
	, mMaxUS( us_max )
{
}


OneShot125::~OneShot125()
{
}


void OneShot125::setSpeedRaw( float speed, bool force_hw_update )
{
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

	uint32_t us = mMinUS + (uint32_t)( ( mMaxUS - mMinUS ) * speed );
	mPWM->SetPWMus( us * 800 / 250 );

	if ( force_hw_update ) {
		mPWM->Update();
	}
}


void OneShot125::Disarm()
{
	mPWM->SetPWMus( 100 * 800 / 250 );
	mPWM->Update();
}


void OneShot125::Disable()
{
	mPWM->SetPWMus( 0 );
	mPWM->Update();
}
