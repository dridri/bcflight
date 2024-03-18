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

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <pthread.h>
#include <list>
#include <string>
#include "../common/Bus.h"

using namespace std;

LUA_CLASS class Serial : public Bus
{
public:
	Serial( const string& device = "", int speed = 9600, bool singleWire = false );
	~Serial();

	int Connect();
	int Read( void* buf, uint32_t len );
	int Write( const void* buf, uint32_t len );
	int Read( uint8_t reg, void* buf, uint32_t len );
	int Write( uint8_t reg, const void* buf, uint32_t len );

	std::string toString();

private:
	int mFD;
	LUA_PROPERTY("single_wire") bool mSingleWire;
	LUA_PROPERTY("device") std::string mDevice;
	LUA_PROPERTY("speed") uint32_t mSpeed;
};

#endif // SERIAL_H

