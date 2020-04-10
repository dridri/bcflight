#include "DShot.h"
#include "Config.h"

#ifdef BUILD_DShot


int DShot::flight_register( Main* main )
{
	RegisterMotor( "DShot", DShot::Instanciate );
	return 0;
}


Motor* DShot::Instanciate( Config* config, const string& object )
{
	int fl_pin = config->Integer( object + ".pin" );
	int fl_min = config->Integer( object + ".minimum_us", 1020 );
	int fl_max = config->Integer( object + ".maximum_us", 1860 );
	return new DShot( fl_pin );
}


DShot::DShot( uint32_t pin )
	: Motor()
	, mPWM( /*TODO*/ )
{
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

	if ( force_hw_update ) {
		mPWM->Update();
	}
}


void DShot::Disable()
{
	mPWM->Update();
}


void DShot::Disarm()
{
	mPWM->Update();
}


#endif // BUILD_DShot
