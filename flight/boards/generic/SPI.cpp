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
{
}


SPI::~SPI()
{
}


std::string SPI::toString()
{
	return mDevice;
}


const string& SPI::device() const
{
	return mDevice;
}


int SPI::Transfer( void* tx, void* rx, uint32_t len )
{
	mTransferMutex.lock();
	int ret = len; // TODO : transfer here
	mTransferMutex.unlock();
	return ret;
}


int SPI::Read( void* buf, uint32_t len )
{
	uint8_t unused[1024];
	memset( unused, 0, len + 1 );
	return Transfer( unused, buf, len );
}


int SPI::Write( const void* buf, uint32_t len )
{
	uint8_t unused[1024];
	memset( unused, 0, len + 1 );
	return Transfer( (void*)buf, unused, len );
}


int SPI::Read( uint8_t reg, void* buf, uint32_t len )
{
	uint8_t tx[1024];
	uint8_t rx[1024];
	memset( tx, 0, len + 1 );
	tx[0] = reg;
	int ret = Transfer( tx, rx, len + 1 );
	memcpy( buf, rx + 1, len );
	return ret;
}


int SPI::Write( uint8_t reg, const void* buf, uint32_t len )
{
	uint8_t tx[1024];
	uint8_t rx[1024];
	tx[0] = reg;
	memcpy( tx + 1, buf, len );
	return Transfer( tx, rx, len + 1 );
}
