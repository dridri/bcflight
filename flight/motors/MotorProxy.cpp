#ifdef BUILD_MotorProxy

#include "MotorProxy.h"
#include "Config.h"
#include "Bus.h"
#include "SPI.h"
#include "I2C.h"
#include "Slave.h"
#include <Board.h>
#include <string>

#define min( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define max( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

using namespace STD;


int MotorProxy::flight_register( Main* main )
{
	RegisterMotor( "Proxy", MotorProxy::Instanciate );
	return 0;
}


Motor* MotorProxy::Instanciate( Config* config, const string& object )
{
	Bus* bus = nullptr;

	string bus_s = config->String( object + ".bus" );
	if ( bus_s.substr(0,3) == "spi" or bus_s.substr(0,3) == "SPI" ) {
		bus = new SPI( bus_s, 0 );
	} else if ( bus_s.substr(0,3) == "i2c" or bus_s.substr(0,3) == "I2C" ) {
		bus = new I2C( config->Integer( object + ".address" ) );
	}

	return new MotorProxy( bus );
}


MotorProxy::MotorProxy( Bus* bus )
	: Motor()
	, mBus( bus )
{
}


MotorProxy::~MotorProxy()
{
}


void MotorProxy::Disable()
{
	uint8_t cmd = Slave::Cmd::MOTOR_DISABLE;
	mBus->Write( &cmd, 1 );
}


void MotorProxy::Disarm()
{
	uint8_t cmd = Slave::Cmd::MOTOR_DISARM;
	mBus->Write( &cmd, 1 );
}


void MotorProxy::setSpeedRaw( float speed, bool force_hw_update )
{
	uint16_t value = max( 0.0f, min( 1.0f, speed ) ) * 65535.0f;
	value = board_htons(value);
	mBus->Write( Slave::Cmd::MOTOR_SET, &value, sizeof(value) );
}


#endif // MotorProxy
