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

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <mutex>
#include "SPI.h"

static std::mutex spi_mutex;

SPI::SPI( const std::string& device, uint32_t speed_hz )
{
	spi_mutex.lock();

	mFD = open( device.c_str(), O_RDWR );
	std::cout << "SPI fd : " << mFD << "\n";

	uint8_t mode, lsb;

	mode = SPI_MODE_0;
	mBitsPerWord = 8;

	if ( ioctl( mFD, SPI_IOC_WR_MODE, &mode ) < 0 )
	{
		std::cout << "SPI wr_mode\n";
	}
	if ( ioctl( mFD, SPI_IOC_RD_MODE, &mode ) < 0 )
	{
		std::cout << "SPI rd_mode\n";
	}
	if ( ioctl( mFD, SPI_IOC_WR_BITS_PER_WORD, &mBitsPerWord ) < 0 ) 
	{
		std::cout << "SPI write bits_per_word\n";
	}
	if ( ioctl( mFD, SPI_IOC_RD_BITS_PER_WORD, &mBitsPerWord ) < 0 ) 
	{
		std::cout << "SPI read bits_per_word\n";
	}
	if ( ioctl( mFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz ) < 0 )  
	{
		std::cout << "can't set max speed hz\n";
	}

	if ( ioctl( mFD, SPI_IOC_RD_MAX_SPEED_HZ, &speed_hz ) < 0 ) 
	{
		std::cout << "SPI max_speed_hz\n";
	}
	if ( ioctl( mFD, SPI_IOC_RD_LSB_FIRST, &lsb ) < 0 )
	{
		std::cout << "SPI rd_lsb_fist\n";
	}

    std::cout << "SPI " << device.c_str() << ":" << mFD <<": spi mode " << (int)mode << ", " << mBitsPerWord << "bits " << ( lsb ? "(lsb first) " : "" ) << "per word, " << speed_hz << " Hz max\n";

	memset( mXFer, 0, sizeof( mXFer ) );
	for ( uint32_t i = 0; i < sizeof( mXFer ) /  sizeof( struct spi_ioc_transfer ); i++) {
		mXFer[i].len = 0;
		mXFer[i].cs_change = 0; // Keep CS activated
		mXFer[i].delay_usecs = 0;
		mXFer[i].speed_hz = speed_hz;
		mXFer[i].bits_per_word = 8;
	}

	spi_mutex.unlock();
}


SPI::~SPI()
{
}


int SPI::Transfer( void* tx, void* rx, uint32_t len )
{
	spi_mutex.lock();

	mXFer[0].tx_buf = (uintptr_t)tx;
	mXFer[0].len = len;
	mXFer[0].rx_buf = (uintptr_t)rx;
	int ret = ioctl( mFD, SPI_IOC_MESSAGE(1), mXFer );

	spi_mutex.unlock();
	return ret;
}
