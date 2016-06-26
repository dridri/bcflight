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

#ifndef STABILIZER_H
#define STABILIZER_H

#include <Frame.h>
#include "PID.h"

class Main;

class Stabilizer
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	Stabilizer( Main* main, Frame* frame );
	~Stabilizer();

	void setP( float p );
	void setI( float i );
	void setD( float d );
	Vector3f getPID() const;
	Vector3f lastPIDOutput() const;

	void setOuterP( float p );
	void setOuterI( float i );
	void setOuterD( float d );
	Vector3f getOuterPID() const;
	Vector3f lastOuterPIDOutput() const;

	void setHorizonOffset( const Vector3f& v );
	Vector3f horizonOffset() const;

	void setMode( uint32_t mode );
	uint32_t mode() const;
	void setAltitudeHold( bool enabled );
	bool altitudeHold() const;

	void CalibrateESCs();
	void Reset( const float& yaw );
	void Update( IMU* imu, Controller* ctrl, float dt );

private:
	Frame* mFrame;
	Mode mMode;
	float mRateFactor;
	bool mAltitudeHold;

	PID mRatePID;
	PID mHorizonPID;
	PID mAltitudePID;
	float mAltitudeControl;

	int mLockState;
	Vector3f mHorizonMultiplier;
	Vector3f mHorizonOffset;
};

#endif // STABILIZER_H
