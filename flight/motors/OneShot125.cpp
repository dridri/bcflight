#include <cmath>
#include "OneShot125.h"
#include "Config.h"
#include "Debug.h"

#ifdef BUILD_OneShot125

#if (defined(BOARD_rpi) || defined(BOARD_generic))
// 	#define TIME_BASE 34500000
// 	#define SCALE ( TIME_BASE / 1000000 )
// 	#define STEPS 10000
// 	#define STEP_US 50
	#define TIME_BASE 62500000
	#define STEPS 9000
	#define SCALE ( STEPS / 250 )
	#define STEP_US 25
#elif defined( BOARD_teensy4 )
	#define TIME_BASE 1000000000
	#define SCALE 1000
	#define STEPS 250000
	#define STEP_US 0 // unused
#endif


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
	PWM* pwm = static_cast<PWM*>( config->Object( object + ".pwm" ) );
	return new OneShot125( fl_pin, fl_min, fl_max, pwm );
}


OneShot125::OneShot125( uint32_t pin, int us_min, int us_max, PWM* pwm )
	: Motor()
	, mPWM( pwm )
	, mPin( pin )
	, mMinUS( us_min )
	, mMaxUS( us_max )
	, mScale( SCALE )
{
	fDebug( pin, us_min, us_max, pwm );
	gDebug() << this;
	if ( !mPWM ) {
	#ifdef BOARD_rpi
		if ( PWM::HasTruePWM() ) {
			mPWM = new PWM( pin, 10 * 1000 * 1000, 2500, 1 );
			mScale = 10;
		} 
		else
	#endif
		{
			mPWM = new PWM( pin, TIME_BASE, STEPS, STEP_US );
		}
	}
}


OneShot125::OneShot125( const LuaValue& pin )
	: OneShot125( pin.toInteger(), 125, 250-8, nullptr )
{
}


OneShot125::~OneShot125()
{
}


void OneShot125::setSpeedRaw( float speed, bool force_hw_update )
{
	fDebug( speed, force_hw_update );
	if ( isnan( speed ) or isinf( speed ) ) {
		return;
	}
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

	uint32_t us = mMinUS * mScale + (uint32_t)( (float)( mMaxUS - mMinUS ) * speed * (float)mScale );
	mPWM->SetPWMus( us );

	if ( force_hw_update ) {
		mPWM->Update();
	}
}


void OneShot125::Disarm()
{
	// mPWM->SetPWMus( 100 * mScale );
	mPWM->SetPWMus( 0 );
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
