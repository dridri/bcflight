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

#include <unistd.h>
#include <cmath>
#include <Debug.h>
#include "IMU.h"
#include "Config.h"
#include "Stabilizer.h"
#include "Accelerometer.h"
#include "Gyroscope.h"
#include "Magnetometer.h"
#include "Altimeter.h"
#include "GPS.h"
#include "MahonyAHRS.h"
#include <Controller.h>

IMU::IMU()
	: mMain( Main::instance() )
	, mSensorsUpdateSlow( 0 )
	, mPositionUpdate( false )
	, mRatesFilter( nullptr )
	, mAccelerometerFilter( nullptr )
	, mState( Off )
	, mAcceleration( Vector3f() )
	, mGyroscope( Vector3f() )
	, mMagnetometer( Vector3f() )
	, mGPSLocation( Vector2f() )
	, mGPSAltitude( 0.0f )
	, mGPSSpeed( 0.0f )
	, mAltitudeOffset( 0.0f )
	, mProximity( 0.0f )
	, mRPY( Vector3f() )
	, mdRPY( Vector3f() )
	, mRate( Vector3f() )
	, mRPYOffset( Vector3f() )
	, mCalibrationStep( 0 )
	, mCalibrationTimer( 0 )
	, mRPYAccum( Vector4f() )
	, mGravity( Vector3f() )
	// , mRates( EKF( 3, 3 ) )
	// , mAccelerationSmoother( EKF( 3, 3 ) )
	, mAttitude( nullptr )
	, mPosition( EKF( 3, 3 ) )
	, mVelocity( EKF( 3, 3 ) )
	, mLastAccelAttitude( Vector4f() )
	, mLastAcceleration( Vector3f() )
	, mGyroscopeErrorCounter( 0 )
{
	/** mPosition matrix :
	 *   - Inputs :
	 *     - XY velocity integrated over time 0 1
	 *     - altitude (either absolute or proximity) 2
	 *   - Outputs :
	 *     - smoothed position 0 1
	 *     - smoothed altitude 2
	 **/
	mPosition.setSelector( 0, 0, 1.0f );
	mPosition.setSelector( 1, 1, 1.0f );
	mPosition.setSelector( 2, 2, 1.0f );

	/** mVelocity matrix :
	 *   - Inputs :
	 *     - smoothed linear acceleration 0 1 2
	 *   - Outputs :
	 *     - smoothed linear velocity 0 1 2
	 **/
	mVelocity.setSelector( 0, 0, 1.0f );
	mVelocity.setSelector( 1, 1, 1.0f );
	mVelocity.setSelector( 2, 2, 1.0f );

	mMain->blackbox()->Enqueue( "IMU:state", "Off" );
}


IMU::~IMU()
{
}


const IMU::State& IMU::state() const
{
	return mState;
}


const Vector3f IMU::RPY() const
{
	return mRPY;
}


const Vector3f IMU::dRPY() const
{
	return mdRPY;
}


const Vector3f IMU::rate() const
{
	return mRate;
}


const float IMU::altitude() const
{
	return mPosition.state( 0 ).z;
}


const Vector3f IMU::velocity() const
{
	return mVelocity.state( 0 );
}


const Vector3f IMU::position() const
{
	return mPosition.state( 0 );
}


const Vector3f IMU::acceleration() const
{
	return mAccelerationSmoothed;
}


const Vector3f IMU::gyroscope() const
{
	return mGyroscope;
}


const Vector3f IMU::magnetometer() const
{
	return mMagnetometer;
}


const Vector3f IMU::gpsLocation() const
{
	return Vector3f( mGPSLocation.x, mGPSLocation.y, mGPSAltitude );
}


const float IMU::gpsSpeed() const
{
	return mGPSSpeed;
}


const uint32_t IMU::gpsSatellitesSeen() const
{
	return mGPSSatellitesSeen;
}


const uint32_t IMU::gpsSatellitesUsed() const
{
	return mGPSSatellitesUsed;
}


void IMU::setPositionFilterInput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mPosition.setInputFilter( 0, v.x );
	mPosition.setInputFilter( 1, v.y );
	mPosition.setInputFilter( 2, v.z );
}


void IMU::setPositionFilterOutput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mPosition.setOutputFilter( 0, v.x );
	mPosition.setOutputFilter( 1, v.y );
	mPosition.setOutputFilter( 2, v.z );
}


void IMU::registerConsumer( const std::function<void(uint64_t, const Vector3f&, const Vector3f&)>& f )
{
	mConsumers.push_back( f );
}


void IMU::Loop( uint64_t tick, float dt )
{
	// fDebug( dt, mState );
	if ( mState == Off ) {
		// Just update GPS
		UpdateGPS();
		// Nothing more to do
	} else if ( mState == Calibrating or mState == CalibratingAll ) {
		Calibrate( dt, ( mState == CalibratingAll ) );
	} else if ( mState == CalibrationDone ) {
		mState = Running;
		mMain->blackbox()->Enqueue( "IMU:state", "Running" );
	} else if ( mState == Running ) {
		UpdateSensors( tick, ( mMain->stabilizer()->mode() == Stabilizer::Rate ) );
		UpdateAttitude( dt );
		if ( mPositionUpdate ) {
#ifdef SYSTEM_NAME_Linux
			mPositionUpdateMutex.lock();
#endif
			mPositionUpdate = false;
#ifdef SYSTEM_NAME_Linux
			mPositionUpdateMutex.unlock();
#endif
			UpdatePosition( dt );
		}
	}

}


void IMU::Calibrate( float dt, bool all )
{
	switch ( mCalibrationStep ) {
		case 0 : {
			gDebug() << "Calibrating " << ( all ? "all" : "partial" ) << " sensors";
			mMain->blackbox()->Enqueue( "IMU:state", "Calibrating" );
			mCalibrationStep++;
			mCalibrationTimer = Board::GetTicks();
			break;
		}
		case 1 : {
			for ( auto dev : Sensor::Devices() ) {
				if ( all or dynamic_cast< Gyroscope* >( dev ) != nullptr or dynamic_cast< Altimeter* >( dev ) != nullptr ) {
					dev->Calibrate( dt, false );
				}
			}
			if ( Board::GetTicks() - mCalibrationTimer >= 1000 * 1000 * 2 ) {
				mCalibrationStep++;
			}
			break;
		}
		case 2 : {
			gDebug() << "Calibration last pass";
			for ( auto dev : Sensor::Devices() ) {
				if ( all or dynamic_cast< Gyroscope* >( dev ) != nullptr or dynamic_cast< Altimeter* >( dev ) != nullptr ) {
					dev->Calibrate( dt, true );
				}
			}
			mCalibrationStep++;
			if ( all == false ) {
				mCalibrationStep = 5;
			}
			break;
		}
		case 3 : {
			gDebug() << "Calibrating gravity";
			mGravity = Vector3f();
			mCalibrationStep++;
			mCalibrationTimer = Board::GetTicks();
			break;
		}
		case 4 : {
			UpdateSensors( dt );
			mGravity = ( mGravity * 0.5f ) + ( mAcceleration * 0.5f );
			if ( Board::GetTicks() - mCalibrationTimer >= 1000 * 1000 * 1 ) {
				mCalibrationStep++;
			}
			break;
		}
		case 5 : {
			gDebug() << "Calibration almost done...";
			mState = CalibrationDone;
			mMain->blackbox()->Enqueue( "IMU:state", "CalibrationDone" );
			mAcceleration = Vector3f();
			mGyroscope = Vector3f();
			mMagnetometer = Vector3f();
			mRPY = Vector3f();
			mdRPY = Vector3f();
			mRate = Vector3f();
			gDebug() << "Calibration done !";
			mMain->frame()->Disarm(); // Activate motors
			break;
		}
		default: break;
	}
}


void IMU::Recalibrate()
{
	bool cal_all = false;
	for ( Accelerometer* dev : Sensor::Accelerometers() ) {
		if ( not dev->calibrated() ) {
			cal_all = true;
			break;
		}
	}
	if ( cal_all ) {
		gDebug() << "Full recalibration needed...";
		RecalibrateAll();
		return;
	}

	mCalibrationStep = 0;
	mState = Calibrating;
	mRPYOffset = Vector3f();
	gDebug() << "Calibrating gyroscope...";
}


void IMU::RecalibrateAll()
{
	mCalibrationStep = 0;
	mState = CalibratingAll;
	mRPYOffset = Vector3f();
	gDebug() << "Calibrating all sensors...";
}


void IMU::ResetRPY()
{
	mAcceleration = Vector3f();
	mRPY = Vector3f();
}


void IMU::ResetYaw()
{
	mRPY.z = 0.0f;
/*
	mVirtualNorth = Vector3f();
	while ( mVirtualNorth.x == 0.0f and mVirtualNorth.y == 0.0f and mVirtualNorth.z == 0.0f ) {
		usleep( 1000 );
	}
*/
}


void IMU::UpdateSensors( uint64_t tick, bool gyro_only )
{
	Vector4f total_accel;
	Vector4f total_gyro;
	Vector4f total_magn;
	Vector2f total_alti;
	Vector2f total_proxi;
	Vector3f total_lat_lon;
	Vector3f vtmp;
	float ftmp;
	char stmp[64];

	for ( Gyroscope* dev : mGyroscopes ) {
		vtmp.x = vtmp.y = vtmp.z = 0.0f;
		int ret = dev->Read( &vtmp );
		if ( ret > 0 ) {
			total_gyro += Vector4f( vtmp, 1.0f );
		}
	}

	if ( total_gyro.w > 0.0f ) {
		mGyroscope = total_gyro.xyz() / total_gyro.w;
	}

	if ( mState == Running and ( not gyro_only or mAcroRPYCounter == 0 ) )
	{
		for ( Accelerometer* dev : mAccelerometers ) {
			vtmp.x = vtmp.y = vtmp.z = 0.0f;
			dev->Read( &vtmp );
			total_accel += Vector4f( vtmp, 1.0f );
		}
		if ( total_accel.w > 0.0f ) {
			mLastAcceleration = mAcceleration;
			mAcceleration = total_accel.xyz() / total_accel.w;
		}

		if ( mSensorsUpdateSlow % 2 == 0 ) {
			for ( Magnetometer* dev : mMagnetometers ) {
				vtmp.x = vtmp.y = vtmp.z = 0.0f;
				dev->Read( &vtmp );
				total_magn += Vector4f( vtmp, 1.0f );
			}
			if ( total_magn.w > 0.0f ) {
				mMagnetometer = total_magn.xyz() / total_magn.w;
			}
			// sprintf( stmp, "\"%.4f,%.4f,%.4f\"", mMagnetometer.x, mMagnetometer.y, mMagnetometer.z );
			// mMain->blackbox()->Enqueue( "IMU:magnetometer", stmp );

			// TODO : handle Altimeters
/*
			for ( Altimeter* dev : mAltimeters ) {
				ftmp = 0.0f;
				dev->Read( &ftmp );
				if ( dev->type() == Altimeter::Proximity and ftmp > 0.0f ) {
					total_proxi += Vector2f( ftmp, 1.0f );
				} else if ( dev->type() == Altimeter::Absolute and std::abs( ftmp - mAltitude ) < 15.0f ) { // this avoids to use erroneous values, 10 is arbitrary but nothing should be able to move by 10m in only one tick
					total_alti += Vector2f( ftmp, 1.0f );
				}
			}
			if ( total_proxi.y > 0.0f ) {
				mProximity = total_proxi.x / total_proxi.y;
			}
*/
			UpdateGPS();

#ifdef SYSTEM_NAME_Linux
			mPositionUpdateMutex.lock();
#endif
			mPositionUpdate = true;
#ifdef SYSTEM_NAME_Linux
			mPositionUpdateMutex.unlock();
#endif
		}
	}

	for ( auto f : mConsumers ) {
		f( tick, mGyroscope, mAcceleration );
	}

	// Update RPY only at 1/16 update frequency when in Rate mode
	mAcroRPYCounter = ( mAcroRPYCounter + 1 ) % 16;
	mSensorsUpdateSlow = ( mSensorsUpdateSlow + 1 ) % 16;
}


void IMU::UpdateGPS()
{
	if ( mGPSes.size() == 0 ) {
		return;
	}

	char stmp[64];
	Vector2f total_lat_lon;
	float total_alti = 0.0f;
	float total_speed = 0.0f;
	uint32_t total_seen = 0;
	uint32_t total_used = 0;

	for ( GPS* dev : mGPSes ) {
		float lattitude = 0.0f;
		float longitude = 0.0f;
		float altitude = 0.0f;
		float speed = 0.0f;
		bool ret = dev->Read( &lattitude, &longitude, &altitude, &speed );
		if ( ret ) {
			total_lat_lon += Vector2f( lattitude, longitude );
			total_alti += altitude;
			total_speed += speed;
		}
		uint32_t seen = 0;
		uint32_t used = 0;
		ret = dev->Stats( &seen, &used );
		total_seen += seen;
		total_used += used;
	}
	mGPSAltitude = total_alti / mGPSes.size();
	mGPSLocation = total_lat_lon.xy() * ( 1.0f / mGPSes.size() );
	mGPSSpeed = total_speed / mGPSes.size();
	mGPSSatellitesSeen = total_seen;
	mGPSSatellitesUsed = total_used;

	sprintf( stmp, "\"%.7f,%.7f,%.2f,%.4f\"", mGPSLocation.x, mGPSLocation.y, mGPSAltitude, mGPSSpeed );
	mMain->blackbox()->Enqueue( "IMU:gps", stmp );
}


void IMU::UpdateAttitude( float dt )
{
	char stmp[64];

	// Process rates
	if ( mRatesFilter ) {
		mRate = mRatesFilter->filter( mGyroscope, dt );
	} else {
		mRate = mGyroscope;
	}

	// Process acceleration
	if ( mAccelerometerFilter ) {
		mAccelerationSmoothed = mAccelerometerFilter->filter( mAcceleration, dt );
	} else {
		mAccelerationSmoothed = mAcceleration;
	}

	// Process attitude
	mAttitude->UpdateInput( 0, mRate );
	mAttitude->UpdateInput( 1, mAccelerationSmoothed );
	mAttitude->Process( dt );
	Vector4f rpy = mAttitude->state();

/*
	static uint32_t test = 0;
	if ( test++ == 10 ) {
		float heading = 0.0f;
		Vector3f mag = mMagnetometer.xyz();
		mag.normalize();
		if ( mag.y >= 0.0f ) {
			heading = acos( mag.x );
		} else {
			heading = -acos( mag.x );
		}
		test = 0;
		heading = heading * 180.0f / M_PI;
// 		printf( "heading : %.2f [ %.2f, %.2f, %.2f ]\n", heading, mMagnetometer.x, mMagnetometer.y, mMagnetometer.z );
	}
*/

	mdRPY = ( rpy - mRPY ) * dt;
	mRPY = rpy;

	char tmp[64];
	sprintf( tmp, "\"%.6f,%.6f,%.6f\"", mGyroscope.x, mGyroscope.y, mGyroscope.z );
	mMain->blackbox()->Enqueue( "IMU:gyroscope", tmp );
	sprintf( tmp, "\"%.6f,%.6f,%.6f\"", mAcceleration.x, mAcceleration.y, mAcceleration.z );
	mMain->blackbox()->Enqueue( "IMU:accelerometer", tmp );
	sprintf( tmp, "\"%.6f,%.6f,%.6f\"", mRate.x, mRate.y, mRate.z );
	mMain->blackbox()->Enqueue( "IMU:rate", tmp );
	sprintf( stmp, "\"%.6f,%.6f,%.6f\"", mAccelerationSmoothed.x, mAccelerationSmoothed.y, mAccelerationSmoothed.z );
	mMain->blackbox()->Enqueue( "IMU:acceleration", stmp );
	sprintf( tmp, "\"%.6f,%.6f,%.6f\"", mRPY.x, mRPY.y, mRPY.z );
	mMain->blackbox()->Enqueue( "IMU:rpy", tmp );
}


void IMU::UpdateVelocity( float dt )
{
	Vector3f velo = mVelocity.state( 0 );
	// Vector3f accel = mAccelerationSmoother.state( 0 );
	Vector3f accel = mAccelerometerFilter->state();

	mVelocity.UpdateInput( 0, velo.x + accel.x * dt );
	mVelocity.UpdateInput( 1, velo.y + accel.y * dt );
	mVelocity.UpdateInput( 2, velo.z + accel.z * dt );

	// TODO : mix with position, to avoid drifting
	mVelocity.Process( dt );
}


void IMU::UpdatePosition( float dt )
{
/*
	Vector3f position = mVelocity.state( 0 );
	Vector3f velo = mVelocity.state( 0 );

	mPosition.UpdateInput( 0, position.x + velo.x * dt );
	mPosition.UpdateInput( 2, position.y + velo.y * dt );

	float altitude = 0.0f;
	if ( mProximity > 0.0f ) {
		mAltitudeOffset = mAltitude - mProximity;
		altitude = mProximity;
	} else {
		altitude = mAltitude - mAltitudeOffset;
	}
	mPosition.UpdateInput( 2, altitude );
*/
/*
	// Calculate acceleration on Z-axis
	float accel_z = cos( mRPY.x ) * cos( mRPY.y ) * ( mAcceleration.z - mGravity.z );
	// Integrate position on Z-axis
	float pos_z = mPosition.state( 2 ) + accel_z * dt * dt;
	mPosition.UpdateInput( 2, pos_z );
*/

	// mPosition.Process( dt );
}
