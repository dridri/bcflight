#include <cmath>
#include "OneShot125.h"
#include "Config.h"

#ifdef BUILD_OneShot125

#define TIME_BASE 32000000
#define SCALE ( TIME_BASE / 1000000 )
#define STEPS 8000
#define STEP_US 25


int OneShot125::flight_register( Main* main )
{
	RegisterMotor( "OneShot125", OneShot125::Instanciate );
	return 0;
}


Motor* OneShot125::Instanciate( Config* config, const string& object )
{
	int fl_pin = config->Integer( object + ".pin" );
	int fl_min = config->Integer( object + ".minimum_us", 125 );
	int fl_max = config->Integer( object + ".maximum_us", 250-8 );
	return new OneShot125( fl_pin, fl_min, fl_max );
}


OneShot125::OneShot125( uint32_t pin, int us_min, int us_max )
	: Motor()
	, mPWM( new PWM( pin, TIME_BASE, STEPS, STEP_US ) )
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

	uint32_t us = mMinUS * SCALE + (uint32_t)( (float)( mMaxUS - mMinUS ) * speed * (float)SCALE );
	mPWM->SetPWMus( us );

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


#endif // BUILD_OneShot125
