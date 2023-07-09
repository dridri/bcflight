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
#include "BMP280.h"
#include "Lua.h"

#define BMP280_REGISTER_DIG_T1       0x88
#define BMP280_REGISTER_DIG_T2       0x8A
#define BMP280_REGISTER_DIG_T3       0x8C

#define BMP280_REGISTER_DIG_P1       0x8E
#define BMP280_REGISTER_DIG_P2       0x90
#define BMP280_REGISTER_DIG_P3       0x92
#define BMP280_REGISTER_DIG_P4       0x94
#define BMP280_REGISTER_DIG_P5       0x96
#define BMP280_REGISTER_DIG_P6       0x98
#define BMP280_REGISTER_DIG_P7       0x9A
#define BMP280_REGISTER_DIG_P8       0x9C
#define BMP280_REGISTER_DIG_P9       0x9E

#define BMP280_REGISTER_CHIPID       0xD0
#define BMP280_REGISTER_VERSION      0xD1
#define BMP280_REGISTER_SOFTRESET    0xE0

#define BMP280_REGISTER_CAL26        0xE1

#define BMP280_REGISTER_CONTROL      0xF4
#define BMP280_REGISTER_CONFIG       0xF5
#define BMP280_REGISTER_PRESSUREDATA 0xF7
#define BMP280_REGISTER_TEMPDATA     0xFA


BMP280::BMP280()
	: Altimeter()
	, mI2C( new I2C( 0x76 ) )
	, mBaseAltitudeAccum( Vector2f() )
{
	mNames.emplace_back( "BMP280" );
	mNames.emplace_back( "bmp280" );

	mI2C->Read16( BMP280_REGISTER_DIG_T1, &dig_T1 );
	mI2C->Read16( BMP280_REGISTER_DIG_T2, &dig_T2 );
	mI2C->Read16( BMP280_REGISTER_DIG_T3, &dig_T3 );

	mI2C->Read16( BMP280_REGISTER_DIG_P1, &dig_P1 );
	mI2C->Read16( BMP280_REGISTER_DIG_P2, &dig_P2 );
	mI2C->Read16( BMP280_REGISTER_DIG_P3, &dig_P3 );
	mI2C->Read16( BMP280_REGISTER_DIG_P4, &dig_P4 );
	mI2C->Read16( BMP280_REGISTER_DIG_P5, &dig_P5 );
	mI2C->Read16( BMP280_REGISTER_DIG_P6, &dig_P6 );
	mI2C->Read16( BMP280_REGISTER_DIG_P7, &dig_P7 );
	mI2C->Read16( BMP280_REGISTER_DIG_P8, &dig_P8 );
	mI2C->Read16( BMP280_REGISTER_DIG_P9, &dig_P9 );

// 	mI2C->Write8( BMP280_REGISTER_CONTROL, 0x3F );
	mI2C->Write8( BMP280_REGISTER_CONTROL, 0b10110111 );
}


BMP280::~BMP280()
{
}


void BMP280::Calibrate( float dt, bool last_pass )
{
	(void)dt;
	const float seaLevelhPA = 1013.25;
	const float p = ReadPressure();
	if ( p > 0.0f ) {
		float altitude = 44330.0 * ( 1.0 - pow( p / seaLevelhPA, 0.1903 ) );
		mBaseAltitudeAccum += Vector2f( altitude, 1.0f );
	}
	if ( last_pass ) {
		mBaseAltitude = mBaseAltitudeAccum.x / mBaseAltitudeAccum.y;
		mBaseAltitudeAccum = Vector2f();
	}
}


float BMP280::ReadTemperature()
{
	int32_t var1, var2;
	uint32_t adc_T;

	mI2C->Read24( BMP280_REGISTER_TEMPDATA, &adc_T );
	adc_T >>= 4;

	var1 = ( ( ( (adc_T>>3) - ((int32_t)dig_T1<<1) ) ) * ((int32_t)dig_T2) ) >> 11;
	var2 = ( ( ( ( (adc_T>>4) - ((int32_t)dig_T1) ) * ( (adc_T>>4) - ((int32_t)dig_T1) ) ) >> 12 ) * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;

	float T  = (float)( ( t_fine * 5 + 128 ) >> 8 );
	return T / 100.0f;
}


float BMP280::ReadPressure()
{
	int64_t var1, var2, p;
	uint32_t adc_P;

	// Must be done first to get the t_fine variable set up
	ReadTemperature();

	mI2C->Read24( BMP280_REGISTER_PRESSUREDATA, &adc_P );
	adc_P >>= 4;

	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)dig_P6;
	var2 = var2 + ( ( var1 * (int64_t)dig_P5 ) << 17 );
	var2 = var2 + ( ((int64_t)dig_P4) << 35 );
	var1 = ( ( var1 * var1 * (int64_t)dig_P3 ) >> 8 ) + ( ( var1 * (int64_t)dig_P2 ) << 12 );
	var1 = ( ( ( ((int64_t)1) << 47) + var1 ) ) * ((int64_t)dig_P1) >> 33;

	if ( var1 == 0 ) {
		return 0.0f;  // avoid exception caused by division by zero
	}
	p = 1048576 - adc_P;
	p = ( ( ( p << 31 ) - var2 ) * 3125 ) / var1;
	var1 = ( ((int64_t)dig_P9) * (p>>13) * (p>>13) ) >> 25;
	var2 = ( ((int64_t)dig_P8) * p ) >> 19;

	p = ( ( p + var1 + var2 ) >> 8 ) + ( ((int64_t)dig_P7) << 4 );
	return (float)p / 256.0f / 100.0f;
}


void BMP280::Read( float* altitude )
{
	const float seaLevelhPA = 1013.25;
	const float p = ReadPressure();
	if ( p > 0.0f ) {
		*altitude = ( 44330.0 * ( 1.0 - pow( p / seaLevelhPA, 0.1903 ) ) - mBaseAltitude );
	}
}


LuaValue BMP280::infos()
{
	LuaValue ret;

	ret["Bus"] = mI2C->infos();

	return ret;
}
