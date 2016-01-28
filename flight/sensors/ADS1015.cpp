#include <unistd.h>
#include "ADS1015.h"

int ADS1015::flight_register( Main* main )
{
	Device dev = { 0x48, ADS1015::Instanciate };
	mKnownDevices.push_back( dev );
	return 0;
}


Sensor* ADS1015::Instanciate()
{
	return new ADS1015();
}


ADS1015::ADS1015()
	: mI2C( new I2C( 0x48 ) )
	, mRingBuffer{ 0.0f }
	, mRingSum( 0.0f )
	, mRingIndex( 0 )
{
	mNames = { "ADS1015" };
}


ADS1015::~ADS1015()
{
	delete mI2C;
}


void ADS1015::Calibrate( float dt, bool last_pass )
{
	(void)dt;
	(void)last_pass;
}


float ADS1015::Read( int channel )
{
	uint16_t _ret = 0;
	uint32_t ret = 0;
	uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
					  ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
					  ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
					  ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
					  ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
					  ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

	config |= ADS1015_REG_CONFIG_PGA_6_144V;
	config |= ( ADS1015_REG_CONFIG_MUX_SINGLE_0 + ( channel << 12 ) );
	config |= ADS1015_REG_CONFIG_OS_SINGLE;
	config = ( ( config << 8 ) & 0xFF00 ) | ( ( config >> 8 ) & 0xFF );

	mI2C->Write16( ADS1015_REG_POINTER_CONFIG, config );
	usleep( 1000 );
	mI2C->Read16( ADS1015_REG_POINTER_CONVERT, &_ret );

	ret = _ret;
	ret = ( ( ret << 8 ) & 0xFF00 ) | ( ( ret >> 8 ) & 0xFF );
	float fret = (float)( ret * 6.144f / 32768.0f );

	mRingSum -= mRingBuffer[ mRingIndex ];
	mRingBuffer[ mRingIndex ] = fret;
	mRingSum += fret;
	mRingIndex = ( mRingIndex + 1 ) % 16;

	return mRingSum / 16.0f;
}
