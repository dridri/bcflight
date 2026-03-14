#include "DShot.h"
#include "DShotDriver.h"
#include <string>

#ifdef BUILD_DShot

DShot::DShot( uint32_t pin )
	: Motor()
	, mDriver( DShotDriver::instance() )
	, mPin( pin )
{
	mDriver->disablePinValue( mPin );
	mDriver->Update();
}


DShot::~DShot() 
{
}


void DShot::setSpeedRaw( float speed, bool force_hw_update )
{
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

	bool tlm = mRequestTelemetry.exchange( false );
	mDriver->setPinValue( mPin, 48 + ( 2047 - 48 ) * speed, tlm );

	if ( force_hw_update ) {
		mDriver->Update();
	}
}


void DShot::Beep( uint8_t beepMode )
{
	mDriver->setPinValue( mPin, 1 + beepMode % 5, true );
	mDriver->Update();
}


void DShot::Disable()
{
	mDriver->disablePinValue( mPin );
	mDriver->Update();
}


void DShot::Disarm()
{
	mDriver->setPinValue( mPin, 0 );
	mDriver->Update();
}


string DShot::toString()
{
	return "DShot(pin=" + to_string( mPin ) + ")";
}


#endif // BUILD_DShot
