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

#ifndef FAKEACCELEROMETER_H
#define FAKEACCELEROMETER_H

#include <Accelerometer.h>

class FakeAccelerometer : public Accelerometer
{
public:
	FakeAccelerometer( int axisCount = 3, const Vector3f& noiseGain = Vector3f( 0.4f, 0.4f, 0.4f ) );
	~FakeAccelerometer();

	void Read( Vector3f* v, bool raw = false );
	void Calibrate( float dt, bool last_pass = false );

	LuaValue infos();

protected:
	int mAxisCount;
	Vector3f mNoiseGain;
	float mSinCounter;
};

#endif // FAKEACCELEROMETER_H
