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
	mI2C->Read16( 0xAA, AC1 );
	mI2C->Read16( 0xAC, AC2 );
	mI2C->Read16( 0xAE, AC3 );
	mI2C->Read16( 0xB0, AC4 );
	mI2C->Read16( 0xB2, AC5 );
	mI2C->Read16( 0xB4, AC6 );
	mI2C->Read16( 0xB6, VB1 );
	mI2C->Read16( 0xB8, VB2 );
	mI2C->Read16( 0xBA, MB );
	mI2C->Read16( 0xBC, MC );
	mI2C->Read16( 0xBE, MD );

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
}


BMP180::~BMP180()
{
}


void BMP180::Calibrate( float dt, bool last_pass )
{
	(void)dt;
	(void)last_pass;
}


void BMP180::Read( float* altitude )
{
	unsigned char data[3];
	float temperature = 0.0f;
	float pressure = 0.0f;

	// Start temperature sampling
	if ( mI2C->Write8( BMP180_REG_CONTROL, BMP180_COMMAND_TEMPERATURE ) != 0 ) {
		// TODO : return last altitude
		return;
	}
	usleep( 1000 * 5 );

	// Get temperature
	if ( mI2C->Read( BMP180_REG_RESULT, data, 2 ) == 0 ) {
		// TODO : return last altitude
		return;
	}
	float tu = ( data[0] * 256.0 ) + data[1];
	float a = c5 * ( tu - c6 );
	temperature = a + ( mc / ( a + md ) );

	// Start pressure
	if ( mI2C->Write8( BMP180_REG_CONTROL, BMP180_COMMAND_PRESSURE3 ) != 0 ) {
		// TODO : return last altitude
		return;
	}
	usleep( 1000 * 26 );

	// Get pressure
	if ( mI2C->Read( BMP180_REG_RESULT, data, 3 ) != 0 ) {
		// TODO : return last altitude
		return;
	}
	float pu = ( data[0] * 256.0 ) + data[1] + ( data[2] / 256.0 );
	float s = temperature - 25.0;
	float x = ( x2 * pow( s, 2 ) ) + ( x1 * s ) + x0;
	float y = ( y2 * pow( s, 2 ) ) + ( y1 * s ) + y0;
	float z = ( pu - x ) / y;
	pressure = ( p2 * pow( z, 2 ) ) + ( p1 * z ) + p0;


	*altitude = 44330.0 * ( 1.0 - std::pow( P / P0, 1 / 5.255 ) ) );
}
