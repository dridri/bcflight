#include <cmath>
#include "OneShot42.h"
#include "Config.h"

#ifdef BUILD_OneShot42


int OneShot42::flight_register( Main* main )
{
	RegisterMotor( "OneShot42", OneShot42::Instanciate );
	return 0;
}


Motor* OneShot42::Instanciate( Config* config, const string& object )
{
	int fl_pin = config->Integer( object + ".pin" );
	int fl_min = config->Integer( object + ".minimum_us", 42 );
	int fl_max = config->Integer( object + ".maximum_us", 84-4 );
	return new OneShot42( fl_pin, fl_min, fl_max );
}


OneShot42::OneShot42( uint32_t pin, int us_min, int us_max )
	: Motor()
	, mPin( pin )
	, mMinUS( us_min )
	, mMaxUS( us_max )
	, mScale( 0 )
{
#ifdef BOARD_rpi
	// DMA fake-PWM cannot handle these frequencies
	if ( PWM::HasTruePWM() ) {
		mPWM = new PWM( pin, 37500000, 3150, 1 );
		mScale = 37.5f;
		mMinValue = 1575;
	}
#else
	// TODO
#endif
}


OneShot42::~OneShot42()
{
}


void OneShot42::setSpeedRaw( float speed, bool force_hw_update )
{
	if ( isnan( speed ) or isinf( speed ) ) {
		return;
	}
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

	uint32_t us = mMinValue + (uint32_t)( (float)( mMaxUS - mMinUS ) * speed * mScale );
	mPWM->SetPWMus( us );

	if ( force_hw_update ) {
		mPWM->Update();
	}
}


void OneShot42::Disarm()
{
	mPWM->SetPWMus( 36 * mScale );
	mPWM->Update();
}


void OneShot42::Disable()
{
	if ( not mPWM ) {
		return;
	}

	mPWM->SetPWMus( 0 );
	mPWM->Update();
}


#endif // BUILD_OneShot42
