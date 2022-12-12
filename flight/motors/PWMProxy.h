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

#ifndef PWMPROXY_H
#define PWMPROXY_H

#include "Motor.h"
#include "SPI.h"

class Main;

class PWMProxy : public Motor
{
public:
	PWMProxy( uint32_t channel, int us_min = 1060, int us_max = 1860, int us_start = 1000, int us_length = 1000 );
	~PWMProxy();

	void Arm();
	void Disarm();
	void Disable();

	static Motor* Instanciate( Config* config, const string& object );
	static int flight_register( Main* main );

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false );
	SPI* mBus;
	uint8_t mChannel;
	uint32_t mMinUS;
	uint32_t mMaxUS;
	uint32_t mStartUS;
	uint32_t mLengthUS;

	static SPI* testBus;
	static uint8_t testBusTx[10];
};

#endif // PWMPROXY_H
