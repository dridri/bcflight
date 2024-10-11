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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <string.h>
#include "MCP320x.h"

MCP320x::MCP320x( const std::string& devfile )
	: mSPI( new SPI( devfile, 500000 ) )
{
}


MCP320x::~MCP320x()
{
}


void MCP320x::setSmoothFactor( uint8_t channel, float f )
{
	mSmoothFactor[channel] = f;
}


uint16_t MCP320x::Read( uint8_t channel, float dt )
{
	static const int nbx = 4;
	uint32_t b[3] = { 0, 0, 0 };
	uint8_t bx[nbx][3] = { { 0 } };
	uint8_t buf[12];

	buf[0] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[1] = ( ( channel & 0b11 ) << 6 );
	buf[2] = 0x00;
	buf[3] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[4] = ( ( channel & 0b11 ) << 6 );
	buf[5] = 0x00;
	buf[6] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[7] = ( ( channel & 0b11 ) << 6 );
	buf[8] = 0x00;
	buf[9] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[10] = ( ( channel & 0b11 ) << 6 );
	buf[11] = 0x00;

	for ( int i = 0; i < nbx; i++ ) {
		if ( mSPI->Transfer( buf, bx[i], 3 ) < 0 ) {
			return 0;
		}
	}

	uint32_t ret[nbx] = { 0 };
	uint32_t final_ret = 0;
	for ( int i = 0; i < nbx; i++ ) {
		ret[i] = ( ( bx[i][1] & 0b1111 ) << 8 ) | bx[i][2];
	}
	for ( int j = 0; j < nbx; j++ ) {
		for ( int i = 0; i < nbx; i++ ) {
			if ( i != j and std::abs( (int32_t)(ret[i] - ret[j]) ) > 250 ) {
				if ( channel == 3 ) {
// 					printf( "ADC too large (%d, %d - %d)\n", ret[i] - ret[j], ret[i], ret[j] );
				}
				return 0;
			}
		}
		final_ret += ret[j];
	}
	final_ret /= nbx;
	final_ret &= 0xFFFFFFF7;

	// Raw value
	if ( dt == 0.0f ) {
		return final_ret;
	}

	if ( mSmoothFactor.find(channel) != mSmoothFactor.end() ) {
		float smooth = mSmoothFactor[channel];
		if ( smooth > 0.0f and smooth < 1.0f ) {
			float s = smooth * std::max( 0.0f, std::min( 1.0f, dt * 1000.0f ) );
			mLastValue[channel] = std::max( 0.0f, mLastValue[channel] * s + (float)final_ret * ( 1.0f - s ) );
			if ( channel == 3 ) {
// 				printf( "ok : %d\n", (uint16_t)mLastValue[channel] );
			}
			return (uint16_t)mLastValue[channel];
		}
	}

	return final_ret;
}
