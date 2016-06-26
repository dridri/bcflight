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

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <pthread.h>
#include <list>

class I2C
{
public:
	I2C( int addr );
	~I2C();
	const int address() const;

	int Read( uint8_t reg, void* buf, uint32_t len );
	int Write( uint8_t reg, void* buf, uint32_t len );
	int Read8( uint8_t reg, uint8_t* value );
	int Read16( uint8_t reg, uint16_t* value, bool big_endian = false );
	int Read16( uint8_t reg, int16_t* value, bool big_endian = false );
	int Read32( uint8_t reg, uint32_t* value );
	int Write8( uint8_t reg, uint8_t value );
	int Write16( uint8_t reg, uint16_t value );
	int Write32( uint8_t reg, uint32_t value );

	static std::list< int > ScanAll();

private:
	int mAddr;
	static int mFD;
	static int mCurrAddr;
	static pthread_mutex_t mMutex;
	int _Read( int addr, uint8_t reg, void* buf, uint32_t len );
	int _Write( int addr, uint8_t reg, void* buf, uint32_t len );
};

#endif
