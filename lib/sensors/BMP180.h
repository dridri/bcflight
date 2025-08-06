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

#ifndef BMP180_H
#define BMP180_H

#include <I2C.h>
#include "Altimeter.h"


// TODO : backport automatic LUA class system
class BMP180 : public Altimeter
{
public:
	BMP180();
	~BMP180();

	void Calibrate( float dt, bool last_pass = false );
	void Read( float* altitude );

	static Sensor* Instanciate( Config* config, const string& object, Bus* bus );

private:
	float ReadTemperature();
	float ReadPressure();

	I2C* mI2C;
	float mBasePressure;

	int16_t AC1, AC2, AC3, VB1, VB2, MB, MC, MD;
	uint16_t AC4, AC5, AC6;
	double c5, c6, mc, md, x0, x1, x2, y0, y1, y2, p0, p1, p2;
	char _error;
};

#endif // BMP180_H
