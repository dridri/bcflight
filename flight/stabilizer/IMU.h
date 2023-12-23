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
#include <Vector.h>
#include <EKF.h>
#include <Filter.h>
#include "SensorFusion.h"
#include <list>
#include <functional>

class Gyroscope;
class Accelerometer;
class Magnetometer;
class Altimeter;
class GPS;

#define IMU_RPY_SMOOTH_RATIO 0.02f

LUA_CLASS class IMU
{
public:
	typedef enum {
		Off,
		Calibrating,
		CalibratingAll,
		CalibrationDone,
		Running
	} State;

	LUA_EXPORT IMU();
	virtual ~IMU();

	// LUA_PROPERTY("filters.rates.input") void setRatesFilterInput( const Vector3f& v );
	// LUA_PROPERTY("filters.rates.output") void setRatesFilterOutput( const Vector3f& v );
	// LUA_PROPERTY("filters.accelerometer.input") void setAccelerometerFilterInput( const Vector3f& v );
	// LUA_PROPERTY("filters.accelerometer.output") void setAccelerometerFilterOutput( const Vector3f& v );
	// LUA_PROPERTY("filters.attitude.input.rates") void setAttitudeFilterRatesInput( const Vector3f& v );
	// LUA_PROPERTY("filters.attitude.input.accelerometer") void setAttitudeFilterAccelerometerInput( const Vector3f& v );
	// LUA_PROPERTY("filters.attitude.output") void setAttitudeFilterOutput( const Vector3f& v );
	LUA_PROPERTY("filters.position.input") void setPositionFilterInput( const Vector3f& v );
	LUA_PROPERTY("filters.position.output") void setPositionFilterOutput( const Vector3f& v );

	const Vector3f acceleration() const;
	const Vector3f gyroscope() const;
	const Vector3f magnetometer() const;

	const State& state() const;
	const Vector3f RPY() const;
	const Vector3f dRPY() const;
	const Vector3f rate() const;
	const Vector3f velocity() const;
	const Vector3f position() const;
	const float altitude() const;
	const Vector3f gpsLocation() const;
	const float gpsSpeed() const;

	LUA_EXPORT void Recalibrate();
	void RecalibrateAll();
	void ResetRPY();
	void ResetYaw();
	void Loop( uint64_t tick, float dt );

	void registerConsumer( const std::function<void(uint64_t, const Vector3f&, const Vector3f&)>& f );

protected:
	void Calibrate( float dt, bool all = false );
	void UpdateSensors( uint64_t tick, bool gyro_only = false );
	void UpdateAttitude( float dt );
	void UpdateVelocity( float dt );
	void UpdatePosition( float dt );
	void UpdateGPS();

	Main* mMain;
	uint32_t mSensorsUpdateSlow;
	bool mPositionUpdate;
#ifdef SYSTEM_NAME_Linux
	mutex mPositionUpdateMutex;
#endif

	LUA_PROPERTY("gyroscopes") std::list<Gyroscope*> mGyroscopes;
	LUA_PROPERTY("accelerometers") std::list<Accelerometer*> mAccelerometers;
	LUA_PROPERTY("magnetometers") std::list<Magnetometer*> mMagnetometers;
	LUA_PROPERTY("altimeters") std::list<Altimeter*> mAltimeters;
	LUA_PROPERTY("gps") std::list<GPS*> mGPSes;

	LUA_PROPERTY("filters.rates") Filter<Vector3f>* mRatesFilter;
	LUA_PROPERTY("filters.accelerometer") Filter<Vector3f>* mAccelerometerFilter;

	// Running states
	State mState;
	Vector3f mAcceleration;
	Vector3f mGyroscope;
	Vector3f mMagnetometer;
	Vector2f mGPSLocation;
	float mGPSSpeed;
	float mGPSAltitude;
	float mAltitudeOffset;
	float mProximity;
	Vector3f mRPY;
	Vector3f mdRPY;
	Vector3f mRate;
	Vector3f mAccelerationSmoothed;
	Vector3f mRPYOffset;
	Vector4f mAccelerometerOffset;

	// Calibration states
	uint32_t mCalibrationStep;
	uint64_t mCalibrationTimer;
	Vector4f mRPYAccum;
	Vector4f mdRPYAccum;
	Vector3f mGravity;

	// EKF mRates;
	// EKF mAccelerationSmoother;
	// EKF mAttitude;
	SensorFusion<Vector3f>* mAttitude;
	EKF mPosition;
	EKF mVelocity;
	Vector4f mLastAccelAttitude;
	Vector3f mVirtualNorth;

	Vector3f mLastAcceleration;
	uint32_t mAcroRPYCounter;

	// Error counters
	uint32_t mGyroscopeErrorCounter;

	std::list< std::function<void(uint64_t, const Vector3f&, const Vector3f&)> > mConsumers;
};

#endif // IMU_H
