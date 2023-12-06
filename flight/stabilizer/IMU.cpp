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
	, mAltitude( 0.0f )
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
	// , mAttitude( EKF( 6, 3 ) )
	, mPosition( EKF( 3, 3 ) )
	, mVelocity( EKF( 3, 3 ) )
	, mLastAccelAttitude( Vector4f() )
	, mLastAcceleration( Vector3f() )
	, mGyroscopeErrorCounter( 0 )
{
	/** mRates matrix :
	 *   - Inputs :
	 *     - gyroscope 0 1 2
	 *   - Outputs :
	 *     - rates 0 1 2
	 **/
	// Set Extended-Kalman-Filter mixing matrix
	// mRates.setSelector( 0, 0, 1.0f );
	// mRates.setSelector( 1, 1, 1.0f );
	// mRates.setSelector( 2, 2, 1.0f );

	/** mAccelerationSmoother matrix :
	 *   - Inputs :
	 *     - acceleration 0 1 2
	 *   - Outputs :
	 *     - smoothed linear acceleration 0 1 2
	 **/
	// Set Extended-Kalman-Filter mixing matrix
	// mAccelerationSmoother.setSelector( 0, 0, 1.0f );
	// mAccelerationSmoother.setSelector( 1, 1, 1.0f );
	// mAccelerationSmoother.setSelector( 2, 2, 1.0f );

	/** mAttitude matrix :
	 *   - Inputs :
	 *     - smoothed linear acceleration 0 1 2
	 *     - rates 3 4 5
	 *   - Outputs :
	 *     - roll-pitch-yaw 0 1 2
	 **/
	// Set Extended-Kalman-Filter mixing matrix (input, output, factor)
/*
	mAttitude.setSelector( 0, 0, 1.0f );
	mAttitude.setSelector( 3, 0, 1.0f );
	mAttitude.setSelector( 1, 1, 1.0f );
	mAttitude.setSelector( 4, 1, 1.0f );
	mAttitude.setSelector( 2, 2, 1.0f );
	mAttitude.setSelector( 5, 2, 1.0f );
*/

	mAttitude = new MahonyAHRS( 0.5f, 0.0f );

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

/*
void IMU::setRatesFilterInput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mRates.setInputFilter( 0, v.x );
	mRates.setInputFilter( 1, v.y );
	mRates.setInputFilter( 2, v.z );
}


void IMU::setRatesFilterOutput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mRates.setOutputFilter( 0, v.x );
	mRates.setOutputFilter( 1, v.y );
	mRates.setOutputFilter( 2, v.z );
}


void IMU::setAccelerometerFilterInput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mAccelerationSmoother.setInputFilter( 0, v.x );
	mAccelerationSmoother.setInputFilter( 1, v.y );
	mAccelerationSmoother.setInputFilter( 2, v.z );
}


void IMU::setAccelerometerFilterOutput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mAccelerationSmoother.setOutputFilter( 0, v.x );
	mAccelerationSmoother.setOutputFilter( 1, v.y );
	mAccelerationSmoother.setOutputFilter( 2, v.z );
}
*/
/*
void IMU::setAttitudeFilterRatesInput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mAttitude.setInputFilter( 3, v.x );
	mAttitude.setInputFilter( 4, v.y );
	mAttitude.setInputFilter( 5, v.z );
}


void IMU::setAttitudeFilterAccelerometerInput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mAttitude.setInputFilter( 0, v.x );
	mAttitude.setInputFilter( 1, v.y );
	mAttitude.setInputFilter( 2, v.z );
}


void IMU::setAttitudeFilterOutput( const Vector3f& v )
{
	fDebug(v.x, v.y, v.z);
	mAttitude.setOutputFilter( 0, v.x );
	mAttitude.setOutputFilter( 1, v.y );
	mAttitude.setOutputFilter( 2, v.z );
}
*/

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
		// Nothing to do
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
	// char stmp[64];

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

	// sprintf( stmp, "\"%.4f,%.4f,%.4f\"", mGyroscope.x, mGyroscope.y, mGyroscope.z );
	// mMain->blackbox()->Enqueue( "IMU:gyroscope", stmp );

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
// 		mAcceleration.normalize();
		// sprintf( stmp, "\"%.4f,%.4f,%.4f\"", mAcceleration.x, mAcceleration.y, mAcceleration.z );
		// gDebug() << "Aceleration : " << stmp;
		// mMain->blackbox()->Enqueue( "IMU:acceleration", stmp );

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

			for ( Altimeter* dev : mAltimeters ) {
				ftmp = 0.0f;
				dev->Read( &ftmp );
				if ( dev->type() == Altimeter::Proximity and ftmp > 0.0f ) {
					total_proxi += Vector2f( ftmp, 1.0f );
				} else if ( dev->type() == Altimeter::Absolute and std::abs( ftmp - mAltitude ) < 15.0f ) { // this avoids to use erroneous values, 10 is arbitrary but nothing should be able to move by 10m in only one tick
					total_alti += Vector2f( ftmp, 1.0f );
				}
			}
			for ( GPS* dev : mGPSes ) {
				float lattitude = 0.0f;
				float longitude = 0.0f;
				float altitude = 0.0f;
				float speed = 0.0f;
				dev->Read( &lattitude, &longitude, &altitude, &speed );
				if ( lattitude != 0.0f and longitude != 0.0f ) {
					total_lat_lon += Vector3f( lattitude, longitude, 1.0f );
				}
				if ( altitude != 0.0f ) {
					total_alti += Vector2f( altitude, 1.0f );
				}
			}
			if ( total_alti.y > 0.0f ) {
				mAltitude = total_alti.x / total_alti.y;
			}
			if ( total_proxi.y > 0.0f ) {
				mProximity = total_proxi.x / total_proxi.y;
			}
			if ( total_lat_lon.z > 0.0f ) {
				mLattitudeLongitude = total_lat_lon.xy() * ( 1.0f / total_lat_lon.z );
			}

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


void IMU::UpdateAttitude( float dt )
{
	// Process rates Extended-Kalman-Filter
	// mRates.UpdateInput( 0, mGyroscope.x );
	// mRates.UpdateInput( 1, mGyroscope.y );
	// mRates.UpdateInput( 2, mGyroscope.z );
	// mRates.Process( dt );
	// mRate = mRates.state( 0 );
	if ( mRatesFilter ) {
		mRate = mRatesFilter->filter( mGyroscope, dt );
	} else {
		mRate = mGyroscope;
	}

	// Process acceleration Extended-Kalman-Filter
	// mAccelerationSmoother.UpdateInput( 0, mAcceleration.x );
	// mAccelerationSmoother.UpdateInput( 1, mAcceleration.y );
	// mAccelerationSmoother.UpdateInput( 2, mAcceleration.z );
	// mAccelerationSmoother.Process( dt );
	// Vector3f accel = mAccelerationSmoother.state( 0 );
	if ( mAccelerometerFilter ) {
		mAccelerationSmoothed = mAccelerometerFilter->filter( mAcceleration, dt );
	} else {
		mAccelerationSmoothed = mAcceleration;
	}
/*
	Vector2f accel_roll_pitch = Vector2f();
	if ( accel.z >= 0.5f * 9.8f ) {
		accel_roll_pitch.x = atan2( accel.x, accel.z ) * 180.0f / M_PI;
		accel_roll_pitch.y = atan2( accel.y, accel.z ) * 180.0f / M_PI;
	} else 
	{
		accel_roll_pitch.x = mRPY.x + mRate.x * dt;
		accel_roll_pitch.y = mRPY.y + mRate.y * dt;
	}
*/
	/*
	if ( abs( accel.x ) >= 0.5f * 9.8f or abs( accel.z ) >= 0.5f * 9.8f ) {
		accel_roll_pitch.x = atan2( accel.x, accel.z ) * 180.0f / M_PI;
	} else {
		accel_roll_pitch.x = mRPY.x + mRate.x * dt;
	}
	if ( abs( accel.y ) >= 0.5f * 9.8f or abs( accel.z ) >= 0.5f * 9.8f ) {
		accel_roll_pitch.y = atan2( accel.y, accel.z ) * 180.0f / M_PI;
	} else {
		accel_roll_pitch.y = mRPY.y + mRate.y * dt;
	}
	*/
/*
	// Update accelerometer values
	mAttitude.UpdateInput( 0, accel_roll_pitch.x );
	mAttitude.UpdateInput( 1, accel_roll_pitch.y );
	if ( mMain->stabilizer()->mode() != Stabilizer::Rate and 0 /TODO/ ) {
// 		mAttitude.UpdateInput( 2, magnetometer.heading );
	} else {
		mAttitude.UpdateInput( 2, mRPY.z + mRate.z * dt );
	}
	// Integrate and update rates values
	mAttitude.UpdateInput( 3, mRPY.x + mRate.x * dt );
	mAttitude.UpdateInput( 4, mRPY.y + mRate.y * dt );
	mAttitude.UpdateInput( 5, mRPY.z + mRate.z * dt );

	// Process Extended-Kalman-Filter
	mAttitude.Process( dt );

	// Retrieve results
	Vector4f rpy = mAttitude.state( 0 );
*/
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
/*
	//TEST
	rpy.x = 0.999f * ( mRPY.x + mRate.x * dt ) + 0.001f * accel_roll_pitch.x;
	rpy.y = 0.999f * ( mRPY.y + mRate.y * dt ) + 0.001f * accel_roll_pitch.y;
	rpy.z = mRPY.z + mRate.z * dt;
*/
	mdRPY = ( rpy - mRPY ) * dt;
	mRPY = rpy;
/*
	char tmp[64];
	sprintf( tmp, "\"%.4f,%.4f,%.4f\"", mRate.x, mRate.y, mRate.z );
	mMain->blackbox()->Enqueue( "IMU:rate", tmp );
	sprintf( tmp, "\"%.4f,%.4f,%.4f\"", mRPY.x, mRPY.y, mRPY.z );
	mMain->blackbox()->Enqueue( "IMU:rpy", tmp );
*/
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
/*
	// Calculate acceleration on Z-axis
	float accel_z = cos( mRPY.x ) * cos( mRPY.y ) * ( mAcceleration.z - mGravity.z );
	// Integrate position on Z-axis
	float pos_z = mPosition.state( 2 ) + accel_z * dt * dt;
	mPosition.UpdateInput( 2, pos_z );
*/

	mPosition.Process( dt );
}
