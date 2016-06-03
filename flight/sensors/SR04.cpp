#include <unistd.h>
#include <Debug.h>
#include <GPIO.h>
#include "SR04.h"


int SR04::flight_register( Main* main )
{
	Device dev;
	dev.iI2CAddr = 0;
	dev.name = "SR04";
	dev.fInstanciate = SR04::Instanciate;
	mKnownDevices.push_back( dev );
	return 0;
}


Sensor* SR04::Instanciate( Config* config, const std::string& object )
{
	SR04* sr04 = new SR04( config->integer( object + ".gpio_trigger" ), config->integer( object + ".gpio_echo" ) );

	return sr04;
}


SR04::SR04( uint32_t gpio_trigger, uint32_t gpio_echo )
	: Altimeter()
	, mTriggerPin( gpio_trigger )
	, mEchoPin( gpio_echo )
	, mAltitude( 0.0f )
{
	mNames.emplace_back( "SR04" );
	mNames.emplace_back( "sr04" );

	GPIO::setMode( mTriggerPin, GPIO::Output );
	GPIO::setMode( mEchoPin, GPIO::Input );
}


SR04::~SR04()
{
}


void SR04::Calibrate( float dt, bool last_pass )
{
}


void SR04::Read( float* altitude )
{
	uint64_t timeout_base = 0;
	uint64_t timeout = 0;

	GPIO::Write( mTriggerPin, true );
	usleep( 10 );
	GPIO::Write( mTriggerPin, false );

	timeout_base = Board::GetTicks();
	timeout = 0;
	while ( GPIO::Read( mEchoPin ) == false and ( timeout = Board::GetTicks() ) - timeout_base < 40000 );
	if ( timeout - timeout_base >= 40000 ) {
		*altitude = 0.0f;
		return;
	}

	uint64_t base_time = Board::GetTicks();

	timeout_base = Board::GetTicks();
	timeout = 0;
	while ( GPIO::Read( mEchoPin ) == true and ( timeout = Board::GetTicks() ) - timeout_base < 40000 );
	uint64_t time = Board::GetTicks() - base_time;

	if ( timeout - timeout_base >= 40000 ) {
		*altitude = 0.0f;
		return;
	}

	*altitude = ( (float)time / 58.0f - 1.0f ) / 100.0f;
}


std::string SR04::infos()
{
	return "";
}
