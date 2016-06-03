#include <unistd.h>
#include <byteswap.h>
#include <cmath>
#include "BMP180.h"

#define	BMP180_REG_CONTROL 0xF4
#define	BMP180_REG_RESULT 0xF6

#define	BMP180_COMMAND_TEMPERATURE 0x2E
#define	BMP180_COMMAND_PRESSURE0 0x34
#define	BMP180_COMMAND_PRESSURE1 0x74
#define	BMP180_COMMAND_PRESSURE2 0xB4
#define	BMP180_COMMAND_PRESSURE3 0xF4

int BMP180::flight_register( Main* main )
{
	Device dev;
	dev.iI2CAddr = 0x77;
	dev.name = "BMP180";
	dev.fInstanciate = BMP180::Instanciate;
	mKnownDevices.push_back( dev );
	return 0;
}


Sensor* BMP180::Instanciate( Config* config, const std::string& object )
{
	return new BMP180();
}


BMP180::BMP180()
	: Altimeter()
	, mI2C( new I2C( 0x77 ) )
{
	mNames.emplace_back( "BMP180" );
	mNames.emplace_back( "bmp180" );

	mI2C->Read16( 0xAA, &AC1, true );
	mI2C->Read16( 0xAC, &AC2, true );
	mI2C->Read16( 0xAE, &AC3, true );
	mI2C->Read16( 0xB0, &AC4, true );
	mI2C->Read16( 0xB2, &AC5, true );
	mI2C->Read16( 0xB4, &AC6, true );
	mI2C->Read16( 0xB6, &VB1, true );
	mI2C->Read16( 0xB8, &VB2, true );
	mI2C->Read16( 0xBA, &MB, true );
	mI2C->Read16( 0xBC, &MC, true );
	mI2C->Read16( 0xBE, &MD, true );

	float c3 = 160.0 * pow( 2, -15 ) * AC3;
	float c4 = pow( 10, -3 ) * pow( 2, -15 ) * AC4;
	float b1 = pow( 160, 2 ) * pow( 2, -30 ) * VB1;
	c5 = ( pow( 2, -15 ) / 160 ) * AC5;
	c6 = AC6;
	mc = ( pow( 2, 11 ) / pow( 160, 2 ) ) * MC;
	md = MD / 160.0;
	x0 = AC1;
	x1 = 160.0 * pow( 2, -13 ) * AC2;
	x2 = pow( 160, 2 ) * pow( 2, -25 ) * VB2;
	y0 = c4 * pow( 2, 15 );
	y1 = c4 * c3;
	y2 = c4 * b1;
	p0 = ( 3791.0 - 8.0 ) / 1600.0;
	p1 = 1.0 - 7357.0 * pow( 2, -20 );
	p2 = 3038.0 * 100.0 * pow( 2, -36 );

// 	mBasePressure = ReadPressure();
	mBasePressure = 1013.45 / std::pow( 1 - ( 65.0 / 44330.0 ), 5.255 );
}


BMP180::~BMP180()
{
}


void BMP180::Calibrate( float dt, bool last_pass )
{
	(void)dt;
	(void)last_pass;
}


float BMP180::ReadTemperature()
{
	int16_t raw_temp = 0;

	// Start temperature sampling
	if ( mI2C->Write8( BMP180_REG_CONTROL, BMP180_COMMAND_TEMPERATURE ) <= 0 ) {
		// TODO : return last temperature
		return 0.0f;
	}
	usleep( 1000 * 5 );

	// Get temperature
	if ( mI2C->Read16( BMP180_REG_RESULT, &raw_temp, true ) == 0 ) {
		// TODO : return last temperature
		return 0.0f;
	}
	float a = c5 * ( (float)raw_temp - c6 );
	return a + ( mc / ( a + md ) );
}


float BMP180::ReadPressure()
{
	unsigned char data[3];
	float temperature = ReadTemperature();

	// Start pressure
	if ( mI2C->Write8( BMP180_REG_CONTROL, BMP180_COMMAND_PRESSURE3 ) <= 0 ) {
		// TODO : return last pressure
		return 0.0f;
	}
	usleep( 1000 * 26 );

	// Get pressure
	if ( mI2C->Read( BMP180_REG_RESULT, data, 3 ) != 3 ) {
		// TODO : return last pressure
		return 0.0f;
	}
	float pu = ( data[0] * 256.0 ) + data[1] + ( data[2] / 256.0 );
	float s = temperature - 25.0;
	float x = ( x2 * pow( s, 2 ) ) + ( x1 * s ) + x0;
	float y = ( y2 * pow( s, 2 ) ) + ( y1 * s ) + y0;
	float z = ( pu - x ) / y;
	return ( p2 * pow( z, 2 ) ) + ( p1 * z ) + p0;
}


void BMP180::Read( float* altitude )
{
	*altitude = 44330.0 * ( 1.0 - std::pow( ReadPressure() / mBasePressure, 1 / 5.255 ) );
}
