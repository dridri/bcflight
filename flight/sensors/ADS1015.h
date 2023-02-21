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

#ifndef ADS1015_H
#define ADS1015_H

#include <I2C.h>
#include "Voltmeter.h"
#include "CurrentSensor.h"

LUA_CLASS class ADS1015 : public Voltmeter
{
public:
	LUA_EXPORT ADS1015();
	~ADS1015();

	void Calibrate( float dt, bool last_pass = false );
	float Read( int channel );

	string infos();

private:
	I2C* mI2C;
	float mRingBuffer[16];
	float mRingSum;
	int mRingIndex;
	bool mChannelReady[4];
};


#endif // ADS1015_H
