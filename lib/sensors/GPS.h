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

#ifndef GPS_H
#define GPS_H

#include <time.h>
#include "Sensor.h"

class GPS : public Sensor
{
public:
	GPS();
	~GPS();

	virtual time_t getTime() = 0;
	virtual bool Read( float* latitude, float* longitude, float* altitude, float* speed ) = 0;
	virtual bool Stats( uint32_t* satSeen, uint32_t* satUsed ) { return false; }
};

#endif // GPS_H
