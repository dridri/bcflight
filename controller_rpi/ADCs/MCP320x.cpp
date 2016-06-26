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

MCP320x::MCP320x()
{
	mFD = open( "/dev/spidev32766.0", O_RDWR );

	uint8_t mode, lsb;
	uint32_t speed = 500000;

	mode = SPI_MODE_0;

	if ( ioctl( mFD, SPI_IOC_WR_MODE, &mode ) < 0 )
	{
		std::cout << "SPI wr_mode\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_RD_MODE, &mode ) < 0 )
	{
		std::cout << "SPI rd_mode\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_WR_BITS_PER_WORD, &mBits ) < 0 ) 
	{
		std::cout << "SPI bits_per_word\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_RD_BITS_PER_WORD, &mBits ) < 0 ) 
	{
		std::cout << "SPI bits_per_word\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed ) < 0 )  
	{
		std::cout << "can't set max speed hz\n";
		return;
	}

	if ( ioctl( mFD, SPI_IOC_RD_MAX_SPEED_HZ, &speed ) < 0 ) 
	{
		std::cout << "SPI max_speed_hz\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_RD_LSB_FIRST, &lsb ) < 0 )
	{
		std::cout << "SPI rd_lsb_fist\n";
		return;
	}

    printf( "/dev/spidev32766.0:%d: spi mode %d, %d bits %sper word, %d Hz max\n", mFD, mode, mBits, lsb ? "(lsb first) " : "", speed );

	memset( mXFer, 0, sizeof( mXFer ) );
	for ( uint32_t i = 0; i < sizeof( mXFer ) /  sizeof( struct spi_ioc_transfer ); i++) {
		mXFer[i].len = 0;
		mXFer[i].cs_change = 0; // Keep CS activated
		mXFer[i].delay_usecs = 0;
		mXFer[i].speed_hz = 500000;
		mXFer[i].bits_per_word = 8;
	}
}


MCP320x::~MCP320x()
{
}


uint16_t MCP320x::Read( uint8_t channel )
{
	static const int nbx = 4;
	uint32_t b[3] = { 0, 0, 0 };
	uint8_t bx[nbx][3] = { { 0 } };
	uint8_t buf[16];

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
		mXFer[0].tx_buf = (uintptr_t)buf;
		mXFer[0].len = 3;
		mXFer[0].rx_buf = (uintptr_t)bx[i];
		int ret = ioctl( mFD, SPI_IOC_MESSAGE(1), mXFer );
	}

	for ( int j = 0; j < nbx; j++ ) {
		for ( int i = 0; i < nbx; i++ ) {
			if ( i != j and bx[i][1] != bx[j][1] ) {
				return 0;
			}
		}
	}
	for ( int i = 0; i < nbx; i++ ) {
		b[0] += bx[i][0];
		b[1] += bx[i][1];
		b[2] += bx[i][2];
	}
	b[0] /= nbx;
	b[1] /= nbx;
	b[2] /= nbx;

	b[2] = b[2] & 0xF7;
/*
	if ( channel != 7 ) {
		if ( b[1] < 0x05 ) {
			return 1500;
		}
		if ( b[1] > 0x09 ) {
			return 2500;
		}
	}
*/
// 	printf( "channel %d : { %02X %02X %02X %04X %d }\n", channel, b[0], b[1], b[2], ( ( b[1] & 0b1111 ) << 8 ) | b[2], ( ( b[1] & 0b1111 ) << 8 ) | b[2] );
	return ( ( b[1] & 0b1111 ) << 8 ) | b[2];

}
