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

#ifdef BOARD_generic

#include <cmath>
#include "FakeGyroscope.h"

FakeGyroscope::FakeGyroscope( int axisCount, const Vector3f& noiseGain )
	: Gyroscope()
	, mAxisCount( axisCount )
	, mNoiseGain( noiseGain )
	, mSinCounter( 0.0f )
{
	mNames = { "FakeGyroscope" };
}


FakeGyroscope::~FakeGyroscope()
{
}


void FakeGyroscope::Calibrate( float dt, bool last_pass )
{
}


int FakeGyroscope::Read( Vector3f* v, bool raw )
{
	for ( int i = 0; i < mAxisCount; i++ ) {
		v->operator[](0) = 0.0f * sin( mSinCounter + 3.14 / 2 ) + mNoiseGain[0] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
		v->operator[](1) = mNoiseGain[1] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
		v->operator[](2) = mNoiseGain[2] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
	}
	mLastValues = *v;
	mSinCounter += 0.1;

	return mAxisCount;
}


LuaValue FakeGyroscope::infos()
{
	LuaValue ret;

	ret["Resolution"] = "32 bits float";

	return ret;
}

#endif // BOARD_generic
