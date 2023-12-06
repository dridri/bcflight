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

LUA_CLASS class Stabilizer
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	LUA_EXPORT Stabilizer();
	~Stabilizer();

	virtual void setRollP( float p );
	virtual void setRollI( float i );
	virtual void setRollD( float d );
	Vector3f getRollPID() const;
	virtual void setPitchP( float p );
	virtual void setPitchI( float i );
	virtual void setPitchD( float d );
	Vector3f getPitchPID() const;
	virtual void setYawP( float p );
	virtual void setYawI( float i );
	virtual void setYawD( float d );
	Vector3f getYawPID() const;
	virtual Vector3f lastPIDOutput() const;

	virtual void setOuterP( float p );
	virtual void setOuterI( float i );
	virtual void setOuterD( float d );
	Vector3f getOuterPID() const;
	virtual Vector3f lastOuterPIDOutput() const;

	virtual void setHorizonOffset( const Vector3f& v );
	virtual Vector3f horizonOffset() const;

	virtual void setMode( uint32_t mode );
	virtual uint32_t mode() const;
	virtual void setAltitudeHold( bool enabled );
	virtual bool altitudeHold() const;

	virtual bool armed() const;
	virtual float thrust() const;
	virtual const Vector3f& RPY() const;
	virtual const Vector3f& filteredRPYDerivative() const;
	virtual void Arm();
	virtual void Disarm();
	virtual void setRoll( float value );
	virtual void setPitch( float value );
	virtual void setYaw( float value );
	virtual void setThrust( float value );

	virtual void CalibrateESCs();
	virtual void MotorTest(uint32_t id);
	virtual void Reset( const float& yaw );
	virtual void Update( IMU* imu, Controller* ctrl, float dt );

	Frame* frame() const;
	void setFrame( Frame* frame );

protected:
	Main* mMain;
	LUA_PROPERTY("frame") Frame* mFrame;
	Mode mMode;
	LUA_PROPERTY("rate_speed") float mRateFactor;
	bool mAltitudeHold;

	LUA_PROPERTY("pid_roll") PID<float> mRateRollPID;
	LUA_PROPERTY("pid_pitch") PID<float> mRatePitchPID;
	LUA_PROPERTY("pid_yaw") PID<float> mRateYawPID;
	LUA_PROPERTY("pid_horizon_roll") PID<float> mRollHorizonPID;
	LUA_PROPERTY("pid_horizon_pitch") PID<float> mPitchHorizonPID;
	PID<float> mAltitudePID;
	float mAltitudeControl;
	Filter<Vector3f>* mDerivativeFilter;

	LUA_PROPERTY("tpa.multiplier") float mTPAMultiplier;
	LUA_PROPERTY("tpa.threshold") float mTPAThreshold;
	LUA_PROPERTY("anti_gravity.gain") float mAntiGravityGain;
	LUA_PROPERTY("anti_gravity.threshold") float mAntiGravityThreshold;
	LUA_PROPERTY("anti_gravity.decay") float mAntiGravityDecay;
	float mAntigravityThrustAccum;

	bool mArmed;
	Vector4f mExpo;
	Vector3f mRPY;
	Vector3f mFilteredRPYDerivative;
	float mThrust;
	float mPreviousThrust;

	int mLockState;
	LUA_PROPERTY("horizon_angles") Vector3f mHorizonMultiplier;
	LUA_PROPERTY("horizon_offset") Vector3f mHorizonOffset;
	LUA_PROPERTY("horizon_max_rate") Vector3f mHorizonMaxRate;
};

#endif // STABILIZER_H
