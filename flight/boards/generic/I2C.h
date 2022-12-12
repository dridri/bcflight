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
#include "../common/Bus.h"

using namespace std;

class I2C : public Bus
{
public:
	I2C( int addr, bool slave = false );
	~I2C();
	const int address() const;

	int Read( void* buf, uint32_t len );
	int Write( const void* buf, uint32_t len );
	int Read( uint8_t reg, void* buf, uint32_t len );
	int Write( uint8_t reg, const void* buf, uint32_t len );

	std::string toString();

	static list< int > ScanAll();

private:
	int mAddr;
};

#endif
