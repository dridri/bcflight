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

#ifndef IMU_H
#define IMU_H

#include <Main.h>
#include <Thread.h>
#include <Vector.h>
#include <EKF.h>

#define IMU_RPY_SMOOTH_RATIO 0.02f

class IMU
{
public:
	typedef enum {
		Off,
		Calibrating,
		CalibratingAll,
		CalibrationDone,
		Running
	} State;

	IMU( Main* main );
	virtual ~IMU();
	void setRateOnly( bool enabled );

	const Vector3f acceleration() const;
	const Vector3f gyroscope() const;
	const Vector3f magnetometer() const;

	const State& state() const;
	const Vector3f RPY() const;
	const Vector3f dRPY() const;
	const Vector3f rate() const;
	const float altitude() const;

	void Recalibrate();
	void RecalibrateAll();
	void ResetRPY();
	void ResetYaw();
	void Loop( float dt );

protected:
	bool SensorsThreadRun();
	void Calibrate( float dt, bool all = false );
	void UpdateSensors( float dt, bool gyro_only = false );
	void UpdateAttitude( float dt );
	void UpdatePosition( float dt );

	Main* mMain;
	HookThread<IMU>* mSensorsThread;
	uint64_t mSensorsThreadTick;
	uint64_t mSensorsThreadTickRate;
	uint32_t mSensorsUpdateSlow;
	bool mPositionUpdate;
	std::mutex mPositionUpdateMutex;

	// Running states
	State mState;
	Vector3f mAcceleration;
	Vector3f mGyroscope;
	Vector3f mMagnetometer;
	float mAltitude;
	float mAltitudeOffset;
	float mProximity;
	Vector3f mRPY;
	Vector3f mdRPY;
	Vector3f mRate;
	Vector3f mRPYOffset;
	Vector4f mAccelerometerOffset;

	// Calibration states
	uint32_t mCalibrationStep;
	uint64_t mCalibrationTimer;
	Vector4f mRPYAccum;
	Vector4f mdRPYAccum;
	Vector3f mGravity;

	EKF mRates;
	EKF mAccelerationSmoother;
	EKF mAttitude;
	EKF mPosition;
	Vector4f mLastAccelAttitude;
	Vector3f mVirtualNorth;

	Vector3f mLastAcceleration;
	uint32_t mAcroRPYCounter;
};

#endif // IMU_H
