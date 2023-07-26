#include "DShot.h"
#include "DShotDriver.h"

#ifdef BUILD_DShot

DShot::DShot( uint32_t pin )
	: Motor()
	, mDriver( DShotDriver::instance() )
	, mPin( pin )
{
	mDriver->disablePinValue( mPin );
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

	mDriver->setPinValue( mPin, 48 + ( 2047 - 48 ) * speed );

	if ( force_hw_update ) {
		mDriver->Update();
	}
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


#endif // BUILD_DShot
