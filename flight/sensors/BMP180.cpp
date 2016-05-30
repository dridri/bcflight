#include "BMP180.h"


int BMP180::flight_register( Main* main )
{
	Device dev = { 0x77, BMP180::Instanciate };
	mKnownDevices.push_back( dev );
	return 0;
}


Sensor* BMP180::Instanciate()
{
	return new BMP180();
}


BMP180::BMP180()
	: Altimeter()
	, mI2C( new I2C( 0x77 ) )
{
}


BMP180::~BMP180()
{
}


void BMP180::Calibrate( float dt, bool last_pass )
{
	// TODO
}


void BMP180::Read( float* altitude )
{
	// TODO
	*altitude = 0.0f;
}
