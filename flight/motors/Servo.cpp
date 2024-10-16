#include "Servo.h"


Servo::Servo( uint32_t pin, uint32_t us_min, uint32_t us_max )
	: mPin( pin )
	, mMinUS( us_min )
	, mMaxUS( us_max )
	, mPWM( new PWM( pin, 1000000, 2000, 2 ) )
{
}


Servo::~Servo()
{
}


void Servo::setValue( float p )
{
	if ( isnan( p ) or isinf( p ) ) {
		return;
	}
	if ( p < 0.0f ) {
		p = 0.0f;
	}
	if ( p > 1.0f ) {
		p = 1.0f;
	}

	uint32_t us = mMinUS + (uint32_t)( ( mMaxUS - mMinUS ) * p );
	mPWM->SetPWMus( us );
	mPWM->Update();
}
