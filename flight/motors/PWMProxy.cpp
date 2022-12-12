/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <unistd.h>
#include <cmath>
#include <Debug.h>
#include "PWMProxy.h"
#include "SPI.h"
#include "Config.h"

#ifdef BUILD_PWMProxy

uint16_t _g_ADCChan1 = 0; // Ugly HACK


SPI* PWMProxy::testBus = nullptr;
uint8_t PWMProxy::testBusTx[] = { 0 };

int PWMProxy::flight_register( Main* main )
{
	RegisterMotor( "PWMProxy", PWMProxy::Instanciate );
	return 0;
}


Motor* PWMProxy::Instanciate( Config* config, const string& object )
{
	int fl_channel = config->Integer( object + ".channel" );
	int fl_min = config->Integer( object + ".minimum_us", 1020 );
	int fl_max = config->Integer( object + ".maximum_us", 1860 );
	int fl_start = config->Integer( object + ".range_start_us", 1000 );
	int fl_length = config->Integer( object + ".range_length_us", 1000 );
	return new PWMProxy( fl_channel, fl_min, fl_max, fl_start, fl_length );
}


PWMProxy::PWMProxy( uint32_t channel, int us_min, int us_max, int us_start, int us_length )
	: Motor()
	, mChannel( channel )
	, mMinUS( us_min )
	, mMaxUS( us_max )
	, mStartUS( us_start )
	, mLengthUS( us_length )
{
	if ( !testBus ) {
		testBus = new SPI( "/dev/spidev3.0", 8000000 );
		usleep( 1000 * 1000 );

		printf( "Send reset\n" );
		testBus->Write( { 0x01, 1, 1, 1, 1, 1, 1, 1, 1 } );
		usleep( 1000 * 1000 );

		printf( "Send resetaaa\n" );
		testBus->Write( { 0x01, 1, 1, 1, 1, 1, 1, 1, 1 } );
		usleep( 1000 * 1000 );

		printf( "Send REG_PWM_CONFIG\n" );
		testBus->Write( { 0x28, 0b00000000, 0, 0, 0, 0, 0, 0, 0 } );
		usleep( 1000 * 1000 );

		printf( "Send minval and maxval (%lu, %lu, n=%lu)\n", us_start, us_length, ( us_max - us_start ) * 65535 / us_length );
		testBus->Write<uint32_t>( 0x30, { us_start, us_length } );
		usleep( 1000 * 1000 * 2 );

		printf( "Send REG_DEVICE_CONFIG\n" );
		testBus->Write( { 0x20, 0b00000001, 0, 0, 0, 0, 0, 0, 0 } );
		usleep( 1000 * 1000 );

		testBusTx[0] = 0x42;
		const uint16_t value = ( mMinUS - mStartUS ) * 65535 / mLengthUS - 10;
		memcpy( &testBusTx[1], &value, sizeof(uint16_t) );
		memcpy( &testBusTx[3], &value, sizeof(uint16_t) );
		memcpy( &testBusTx[5], &value, sizeof(uint16_t) );
		memcpy( &testBusTx[7], &value, sizeof(uint16_t) );
	}
	mBus = testBus;
}


PWMProxy::~PWMProxy()
{
}
// 64045 → 2978

void PWMProxy::setSpeedRaw( float speed, bool force_hw_update )
{
	static uint8_t __attribute__((aligned(32))) dummy[10] = { 0 };

	if ( isnan( speed ) or isinf( speed ) ) {
		return;
	}
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	if ( speed > 1.0f ) {
		speed = 1.0f;
	}

//	uint32_t value = mMinUS + (uint32_t)( ( mMaxUS - mMinUS ) * speed );
// 	uint16_t value = ( mMinUS + (uint32_t)( ( mMaxUS - mMinUS ) * speed ) - mStartUS ) * 65535 / mLengthUS;
	uint16_t value = (uint32_t)( ( (float)mMinUS + ( (float)( mMaxUS - mMinUS ) * speed ) - (float)mStartUS ) * 65535.0f ) / mLengthUS;
// 	gDebug() << "Motor at channel " << (int)mChannel << " speed : " << value << "(" << speed << ")";
	if ( mChannel == 0 ) {
		printf( "%02.6f, %06u, %05.6f, %05.6f\n", speed, value, (float)( mMaxUS - mMinUS ) * speed, ( (float)mMinUS + ( (float)( mMaxUS - mMinUS ) * speed ) - (float)mStartUS ) );
	}
	memcpy( &testBusTx[1 + mChannel*2], &value, sizeof(uint16_t) );

	if ( force_hw_update ) {
// 		uint16_t b[4];
		/*
		uint16_t v = 100;
		memcpy( &testBusTx[1 + 0], &v, 2 );
		memcpy( &testBusTx[1 + 2], &v, 2 );
		memcpy( &testBusTx[1 + 4], &v, 2 );
		memcpy( &testBusTx[1 + 6], &v, 2 );
		*/
// 		memcpy( b, &testBusTx[1], 2 * 4 );
// 		printf( "Transfer %d, %d, %d, %d {", b[0], b[1], b[2], b[3] ); for(int i = 0; i < 9; i++) printf( " %02X", ((uint8_t*)testBusTx)[i]); printf( " }\n" );
		testBus->Transfer( testBusTx, dummy + 1, 9 );
		_g_ADCChan1 = ((uint16_t*)dummy)[1];
	}
}


void PWMProxy::Arm()
{
	testBus->Write( { 0x20, 0b00000101, 0, 0, 0, 0, 0, 0, 0 } );
	usleep( 100 );
}


void PWMProxy::Disarm()
{
	static uint8_t dummy[9] = { 0 };

	uint16_t value = ( mMinUS - mStartUS ) * 65535 / mLengthUS;
	if ( value > 10 ) {
		value -= 10;
	}
// 	gDebug() << "Motor at channel " << (int)mChannel << " speed : " << value << " (disarm)";
	memcpy( &testBusTx[1 + mChannel*2], &value, sizeof(uint16_t) );

	uint16_t b[4];
	memcpy( b, &testBusTx[1], 2 * 4 );
	printf( "DISARM Transfer %d, %d, %d, %d {", b[0], b[1], b[2], b[3] ); for(int i = 0; i < 9; i++) printf( " %02X", ((uint8_t*)testBusTx)[i]); printf( " }\n" );

	testBus->Transfer( testBusTx, dummy, 9 );
	usleep( 100 );

	testBus->Write( { 0x20, 0b00000001, 0, 0, 0, 0, 0, 0, 0 } );
}


void PWMProxy::Disable()
{
	static uint8_t dummy[9] = { 0 };
/*
	uint16_t value = 0;
// 	gDebug() << "Motor at channel " << (int)mChannel << " speed : " << value << " (disable)";
	memcpy( &testBusTx[1 + mChannel*2], &value, sizeof(uint16_t) );
*/
// 	uint16_t b[4];
// 	memcpy( b, &testBusTx[1], 2 * 4 );
// 	printf( "Transfer %d, %d, %d, %d {", b[0], b[1], b[2], b[3] ); for(int i = 0; i < 9; i++) printf( " %02X", ((uint8_t*)testBusTx)[i]); printf( " }\n" );
/*
	testBus->Transfer( testBusTx, dummy, 9 );
*/
	testBus->Write( { 0x20, 0b00000001, 0, 0, 0, 0, 0, 0, 0 } );
	usleep( 100 );
	testBus->Write( { 0x70, 1, 1, 1, 1, 1, 1, 1, 1 } );
	usleep( 100 );
}

#endif // BUILD_PWMProxy
