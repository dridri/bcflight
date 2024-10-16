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

#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <vector>

LUA_CLASS class PWM
{
public:
	LUA_EXPORT PWM( uint32_t pin, uint32_t time_base, uint32_t period_time_us, uint32_t sample_us, bool loop = true );
	~PWM();

	void SetPWMus( uint32_t width_us );
	void Update();

};

#endif // PWM_H
