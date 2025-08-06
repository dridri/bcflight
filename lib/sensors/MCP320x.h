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

#ifndef MCP320X_H
#define MCP320X_H

#include <stdint.h>
#include <SPI.h>
#include "Voltmeter.h"
#include <map>


// TODO : backport automatic LUA class system
class MCP320x : public Voltmeter
{
public:
	MCP320x( const std::string& devfile );
	~MCP320x();

	void setSmoothFactor( int channel, float f );
	uint16_t Read( int channel, float dt );
	virtual float Read( int channel );
	virtual void Calibrate( float dt, bool last_pass = false );

private:
	SPI* mSPI;
	std::map< int, float > mSmoothFactor;
	std::map< int, float > mLastValue;
	uint64_t mLastReadTick;
};

#endif // MCP320X_H
