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
#include <sstream>
#include "I2C.h"

I2C::I2C( int addr, bool slave )
	: Bus()
	, mAddr( addr )
{
}


I2C::~I2C()
{
}


std::string I2C::toString()
{
	stringstream ss;
	ss <<"I2C@0x" << std::hex << mAddr;
	return ss.str();
}


const int I2C::address() const
{
	return mAddr;
}


int I2C::Read( void* buf, uint32_t len )
{
	return 0;
}


int I2C::Write( const void* buf, uint32_t len )
{
	return 0;
}


int I2C::Read( uint8_t reg, void* buf, uint32_t len )
{
	return 0;
}


int I2C::Write( uint8_t reg, const void* _buf, uint32_t len )
{
	return 0;
}


list< int > I2C::ScanAll()
{
	list< int > ret;
	// See boards/rpi/I2C.cpp implementation
	return ret;
}
