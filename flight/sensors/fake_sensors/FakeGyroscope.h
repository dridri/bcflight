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

#ifndef FAKEGYROSCOPE_H
#define FAKEGYROSCOPE_H

#include <Gyroscope.h>

class FakeGyroscope : public Gyroscope
{
public:
	FakeGyroscope( int axisCount = 3, const Vector3f& noiseGain = Vector3f( 0.4f, 0.4f, 0.4f ) );
	~FakeGyroscope();

	int Read( Vector3f* v, bool raw = false );
	void Calibrate( float dt, bool last_pass = false );

	string infos();

protected:
	int mAxisCount;
	Vector3f mNoiseGain;
	float mSinCounter;
};

#endif // FAKEGYROSCOPE_H
