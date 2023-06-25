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

#ifndef GENERIC_H
#define GENERIC_H

#include <PWM.h>
#include "Motor.h"

class Main;

LUA_CLASS class BrushlessPWM : public Motor
{
public:
	LUA_EXPORT BrushlessPWM( uint32_t pin, int us_min = 1060, int us_max = 1860 );
	~BrushlessPWM();

	void Disarm();
	void Disable();

	static Motor* Instanciate( Config* config, const string& object );
	static int flight_register( Main* main );

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false );
	PWM* mPWM;
	uint32_t mMinUS;
	uint32_t mMaxUS;
};

#endif // GENERIC_H
