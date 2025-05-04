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
	: mSPI( new SPI( devfile, 2000000 ) )
{
	mSPI->Connect();
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
	static constexpr int nbx = 1;
	uint8_t bx[3 * nbx] = { 0 };
	uint8_t buf[3 * nbx];

	buf[0] = 0b00000110 | ((channel & 0b100) >> 2);
	buf[1] = (channel & 0b011) << 6;
	buf[2] = 0x00;

	bool ok = false;
	static constexpr int retries = 3;
	// for ( int i = 0; i < nbx; i++ ) {
	for ( int i = 0; i < retries; i++ ) {
		if ( mSPI->Transfer( buf, &bx[i * 3], 3 ) == 3 ) {
			ok = true;
			break;
		}
	}
	if ( not ok ) {
		return 0;
	}

	uint32_t ret[nbx] = { 0 };
	uint32_t final_ret = 0;
	for ( int i = 0; i < nbx; i++ ) {
		ret[i] = ( ( bx[i*3 + 1] & 0b1111 ) << 8 ) | bx[i*3 + 2];
	}
	final_ret = ret[0];

	// int final_nbx = 0;
	// for ( int j = 0; j < nbx; j++ ) {
	// 	bool ignore = false;
	// 	for ( int i = 0; i < nbx; i++ ) {
	// 		if ( i != j and std::abs( (int32_t)(ret[i] - ret[j]) ) > 250 ) {
	// 			ignore = true;
	// 		}
	// 	}
	// 	if ( not ignore ) {
	// 		final_ret += ret[j];
	// 		final_nbx++;
	// 	}
	// }
	// final_ret /= final_nbx;
	// final_ret &= 0xFFFFFFF7;
	// if ( channel >= 4 ) {
	// 	printf( "smooth[%d] : %04d (%d / %d values)\n", channel, final_ret, final_nbx, nbx );
	// }
/*
	if ( channel == 0 ) {
		printf( "\r% 6d", final_ret ); fflush(stdout);
	}
	if ( channel == 1 ) {
		printf( "\r\033[6C% 6d", final_ret ); fflush(stdout);
	}
	if ( channel == 2 ) {
		printf( "\r\033[12C% 6d", final_ret ); fflush(stdout);
	}
	if ( channel == 3 ) {
		printf( "\r\033[18C% 6d", final_ret ); fflush(stdout);
	}
	if ( channel >= 4 ) {
		printf( "\r\033[24C% 6d", final_ret ); fflush(stdout);
	}
*/
	// Raw value
	if ( dt == 0.0f ) {
		return final_ret;
	}

	if ( mSmoothFactor.find(channel) != mSmoothFactor.end() ) {
		float smooth = mSmoothFactor[channel];
		if ( smooth > 0.0f ) {
			float s = smooth * std::max( 0.0f, std::min( 1.0f, dt * 1000.0f ) );
			// float s = smooth * std::max( 0.0f, dt * 1000.0f );
			mLastValue[channel] = std::max( 0.0f, mLastValue[channel] * s + (float)final_ret * ( 1.0f - s ) );
			return (uint16_t)mLastValue[channel];
		}
	}

	return final_ret;
}
