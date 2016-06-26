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

#include <Servo.h>
#include "Motor.h"

class Generic : public Motor
{
public:
	Generic( Servo* servo, float minspeed = 0.0f, float maxSpeed = 1.0f );
	~Generic();

	void Disarm();
	void Disable();

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false );
	Servo* mServo;
	float mMinSpeed;
	float mMaxSpeed;
};

#endif // GENERIC_H
