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

#ifndef SERVO_H
#define SERVO_H

class Servo
{
public:
	Servo( int pin, int us_min = 1060, int us_max = 1860 );
	virtual ~Servo();
// 	void setValue( int width_ms );
	void setValue( float p, bool force_hw_update = false );
	void Disarm();
	void Disable();

	static void HardwareSync();

private:
	int mID;
	int mMin;
	int mMax;
	int mRange;
};

#endif // SERVO_H
