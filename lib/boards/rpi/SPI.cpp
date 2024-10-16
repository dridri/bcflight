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

#include <iostream>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <Debug.h>
#include "SPI.h"
#include "Board.h"

using namespace std;

SPI::SPI( const string& device, uint32_t speed_hz )
	: Bus()
	, mDevice( device )
	, mSpeedHz( speed_hz )
{
	fDebug( device, speed_hz );
}


SPI::~SPI()
{
}


int SPI::Connect()
{
	fDebug( mDevice, mSpeedHz );
	if ( mConnected ) {
		return 0;
	}

	errno = 0;
	mFD = open( mDevice.c_str(), O_RDWR );
	if ( mFD < 0 ) {
		gError() << "fd : " << mFD << " (" << strerror(errno) << ")";
		return -1;
	}

	uint8_t mode, lsb;

	mode = SPI_MODE_0;
	mBitsPerWord = 8;

	if ( ioctl( mFD, SPI_IOC_WR_MODE, &mode ) < 0 )
	{
		gError() << "SPI wr_mode";
		return -1;
	}
	if ( ioctl( mFD, SPI_IOC_RD_MODE, &mode ) < 0 )
	{
		gError() << "SPI rd_mode";
		return -1;
	}
	if ( ioctl( mFD, SPI_IOC_WR_BITS_PER_WORD, &mBitsPerWord ) < 0 ) 
	{
		gError() << "SPI write bits_per_word";
		return -1;
	}
	if ( ioctl( mFD, SPI_IOC_RD_BITS_PER_WORD, &mBitsPerWord ) < 0 ) 
	{
		gError() << "SPI read bits_per_word";
		return -1;
	}
	if ( ioctl( mFD, SPI_IOC_WR_MAX_SPEED_HZ, &mSpeedHz ) < 0 )  
	{
		gError() << "can't set max speed hz";
		return -1;
	}

	if ( ioctl( mFD, SPI_IOC_RD_MAX_SPEED_HZ, &mSpeedHz ) < 0 ) 
	{
		gError() << "SPI max_speed_hz";
		return -1;
	}
	if ( ioctl( mFD, SPI_IOC_RD_LSB_FIRST, &lsb ) < 0 )
	{
		gError() << "SPI rd_lsb_fist";
		return -1;
	}

    gDebug() << mDevice.c_str() << ":" << mFD <<": spi mode " << (int)mode << ", " << mBitsPerWord << "bits " << ( lsb ? "(lsb first) " : "" ) << "per word, " << mSpeedHz << " Hz max";

	memset( mXFer, 0, sizeof( mXFer ) );
	for ( uint32_t i = 0; i < sizeof( mXFer ) /  sizeof( struct spi_ioc_transfer ); i++) {
		mXFer[i].len = 0;
		mXFer[i].cs_change = 0; // Keep CS activated
		mXFer[i].delay_usecs = 0;
		mXFer[i].speed_hz = mSpeedHz;
		mXFer[i].bits_per_word = 8;
	}

	mConnected = true;
	return 0;
}


LuaValue SPI::infos()
{
	LuaValue ret;

	ret["Type"] = "SPI";
	ret["Device"] = mDevice;
	ret["Bitrate"] = to_string(mSpeedHz) + "Hz";

	return ret;
}


const string& SPI::device() const
{
	return mDevice;
}


int SPI::Transfer( void* tx, void* rx, uint32_t len )
{
	mTransferMutex.lock();
	mXFer[0].tx_buf = (uintptr_t)tx;
	mXFer[0].len = len;
	mXFer[0].rx_buf = (uintptr_t)rx;
	int ret = ioctl( mFD, SPI_IOC_MESSAGE(1), mXFer );
	mTransferMutex.unlock();
	return ret;
}


int SPI::Read( void* buf, uint32_t len )
{
	uint8_t unused[256];
	memset( unused, 0, len + 1 );
	return Transfer( unused, buf, len );
}


int SPI::Write( const void* buf, uint32_t len )
{
	uint8_t unused[256];
	memset( unused, 0, len + 1 );
	return Transfer( (void*)buf, unused, len );
}


int SPI::Read( uint8_t reg, void* buf, uint32_t len )
{
	uint8_t tx[256];
	uint8_t rx[256];
	memset( tx, 0, len + 1 );
	tx[0] = reg;
	int ret = Transfer( tx, rx, len + 1 );
	memcpy( buf, rx + 1, len );
	return ret - 1;
}


int SPI::Write( uint8_t reg, const void* buf, uint32_t len )
{
	uint8_t tx[256];
	uint8_t rx[256];
	tx[0] = reg;
	memcpy( tx + 1, buf, len );
	return Transfer( tx, rx, len + 1 ) - 1;
}
