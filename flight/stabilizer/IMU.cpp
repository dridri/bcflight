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
#include <Controller.h>

IMU::IMU( Main* main )
	: mMain( main )
	, mSensorsUpdateSlow( 0 )
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
	, mRates( 3, 3 )
	, mAccelerationSmoother( 3, 3 )
	, mAttitude( 6, 3 )
	, mPosition( 3, 3 )
	, mVelocity( 3, 3 )
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
	mRates.setSelector( 0, 0, 1.0f );
	mRates.setSelector( 1, 1, 1.0f );
	mRates.setSelector( 2, 2, 1.0f );

	// Set gyroscope filtering factors
	mRates.setInputFilter( 0, main->config()->Number( "stabilizer.filters.rates.input.x", 80.0f ) );
	mRates.setInputFilter( 1, main->config()->Number( "stabilizer.filters.rates.input.y", 80.0f ) );
	mRates.setInputFilter( 2, main->config()->Number( "stabilizer.filters.rates.input.z", 80.0f ) );

	// Set output rates filtering factors
	mRates.setOutputFilter( 0, main->config()->Number( "stabilizer.filters.rates.output.x", 0.5f ) );
	mRates.setOutputFilter( 1, main->config()->Number( "stabilizer.filters.rates.output.y", 0.5f ) );
	mRates.setOutputFilter( 2, main->config()->Number( "stabilizer.filters.rates.output.z", 0.5f ) );


	/** mAccelerationSmoother matrix :
	 *   - Inputs :
	 *     - acceleration 0 1 2
	 *   - Outputs :
	 *     - smoothed linear acceleration 0 1 2
	 **/
	// Set Extended-Kalman-Filter mixing matrix
	mAccelerationSmoother.setSelector( 0, 0, 1.0f );
	mAccelerationSmoother.setSelector( 1, 1, 1.0f );
	mAccelerationSmoother.setSelector( 2, 2, 1.0f );

	// Set gyroscope filtering factors
	mAccelerationSmoother.setInputFilter( 0, main->config()->Number( "stabilizer.filters.accelerometer.input.x", 100.0f ) );
	mAccelerationSmoother.setInputFilter( 1, main->config()->Number( "stabilizer.filters.accelerometer.input.y", 100.0f ) );
	mAccelerationSmoother.setInputFilter( 2, main->config()->Number( "stabilizer.filters.accelerometer.input.z", 250.0f ) );

	// Set output rates filtering factors
	mAccelerationSmoother.setOutputFilter( 0, main->config()->Number( "stabilizer.filters.accelerometer.output.x", 0.5f ) );
	mAccelerationSmoother.setOutputFilter( 1, main->config()->Number( "stabilizer.filters.accelerometer.output.y", 0.5f ) );
	mAccelerationSmoother.setOutputFilter( 2, main->config()->Number( "stabilizer.filters.accelerometer.output.z", 0.5f ) );


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
	mAttitude.setSelector( 0, 0, max( 0.01f, min( 1000.0f, main->config()->Number( "stabilizer.filters.attitude.accelerometer.factor.x", 1.0f ) ) ) );
	mAttitude.setSelector( 3, 0, max( 0.01f, min( 1000.0f, main->config()->Number( "stabilizer.filters.attitude.rates.factor.x", 1.0f ) ) ) );
	mAttitude.setSelector( 1, 1, max( 0.01f, min( 1000.0f, main->config()->Number( "stabilizer.filters.attitude.accelerometer.factor.y", 1.0f ) ) ) );
	mAttitude.setSelector( 4, 1, max( 0.01f, min( 1000.0f, main->config()->Number( "stabilizer.filters.attitude.rates.factor.y", 1.0f ) ) ) );
	mAttitude.setSelector( 2, 2, max( 0.01f, min( 1000.0f, main->config()->Number( "stabilizer.filters.attitude.accelerometer.factor.z", 1.0f ) ) ) );
	mAttitude.setSelector( 5, 2, max( 0.01f, min( 1000.0f, main->config()->Number( "stabilizer.filters.attitude.rates.factor.z", 1.0f ) ) ) );

	// Set acceleration filtering factors
	mAttitude.setInputFilter( 0, main->config()->Number( "stabilizer.filters.attitude.input.accelerometer.x", 0.1f ) );
	mAttitude.setInputFilter( 1, main->config()->Number( "stabilizer.filters.attitude.input.accelerometer.y", 0.1f ) );
	mAttitude.setInputFilter( 2, main->config()->Number( "stabilizer.filters.attitude.input.accelerometer.z", 0.1f ) );

	// Set rates filtering factors
	mAttitude.setInputFilter( 3, main->config()->Number( "stabilizer.filters.attitude.input.rates.x", 0.01f ) );
	mAttitude.setInputFilter( 4, main->config()->Number( "stabilizer.filters.attitude.input.rates.y", 0.01f ) );
	mAttitude.setInputFilter( 5, main->config()->Number( "stabilizer.filters.attitude.input.rates.z", 0.01f ) );

	// Set output roll-pitch-yaw filtering factors
	mAttitude.setOutputFilter( 0, main->config()->Number( "stabilizer.filters.attitude.output.z", 0.25f ) );
	mAttitude.setOutputFilter( 1, main->config()->Number( "stabilizer.filters.attitude.output.z", 0.25f ) );
	mAttitude.setOutputFilter( 2, main->config()->Number( "stabilizer.filters.attitude.output.z", 0.25f ) );


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
	mPosition.setInputFilter( 0, main->config()->Number( "stabilizer.filters.position.input.velocity.x", 1.0f ) );
	mPosition.setInputFilter( 1, main->config()->Number( "stabilizer.filters.position.input.velocity.y", 1.0f ) );
	mPosition.setInputFilter( 2, main->config()->Number( "stabilizer.filters.position.input.velocity.z", 10.0f ) );
	mPosition.setOutputFilter( 0, main->config()->Number( "stabilizer.filters.position.output.x", 0.99f ) );
	mPosition.setOutputFilter( 1, main->config()->Number( "stabilizer.filters.position.output.y", 0.99f ) );
	mPosition.setOutputFilter( 2, main->config()->Number( "stabilizer.filters.position.output.z", 0.99f ) );


	/** mVelocity matrix :
	 *   - Inputs :
	 *     - smoothed linear acceleration 0 1 2
	 *   - Outputs :
	 *     - smoothed linear velocity 0 1 2
	 **/
	mVelocity.setSelector( 0, 0, 1.0f );
	mVelocity.setSelector( 1, 1, 1.0f );
	mVelocity.setSelector( 2, 2, 1.0f );

	// Set input velocity filtering factors
	mVelocity.setInputFilter( 0, main->config()->Number( "stabilizer.filters.velocity.input.x", 1.0f ) );
	mVelocity.setInputFilter( 1, main->config()->Number( "stabilizer.filters.velocity.input.y", 1.0f ) );
	mVelocity.setInputFilter( 2, main->config()->Number( "stabilizer.filters.velocity.input.z", 1.0f ) );

	// Set output velocity filtering factors
	mVelocity.setOutputFilter( 0, main->config()->Number( "stabilizer.filters.velocity.output.x", 0.99f ) );
	mVelocity.setOutputFilter( 1, main->config()->Number( "stabilizer.filters.velocity.output.y", 0.99f ) );
	mVelocity.setOutputFilter( 2, main->config()->Number( "stabilizer.filters.velocity.output.z", 0.99f ) );

	mSensorsThreadTick = 0;
	mSensorsThreadTickRate = 0;
// 	mSensorsThread = new HookThread<IMU>( "imu_sensors", this, &IMU::SensorsThreadRun );
// 	mSensorsThread->Start();
// 	mSensorsThread->setPriority( 99 );

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
	return mPosition.state( 0 ).x;
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
	return mAcceleration;
}


const Vector3f IMU::gyroscope() const
{
	return mGyroscope;
}


const Vector3f IMU::magnetometer() const
{
	return mMagnetometer;
}


void IMU::setRateOnly( bool enabled )
{
	fDebug( enabled );
	if ( enabled ) {
// 		mSensorsThread->Pause();
	} else {
// 		mSensorsThread->Start();
	}
}


// static int imu_fps = 0;
// static uint64_t imu_ticks = 0;
bool IMU::SensorsThreadRun()
{
/*
	if ( mState == Running ) {
		float dt = ((float)( Board::GetTicks() - mSensorsThreadTick ) ) / 1000000.0f;
		mSensorsThreadTick = Board::GetTicks();
		if ( abs( dt ) >= 1.0 ) {
			gDebug() << "Critical : dt too high !! ( " << dt << " )\n";
			return true;
		}

		bool rate = ( mMain->stabilizer()->mode() == Stabilizer::Rate );
		UpdateSensors( dt, rate );
		mSensorsThreadTickRate = Board::WaitTick( 1000000 / 500, mSensorsThreadTickRate, -200 );

		imu_fps++;
		if ( Board::GetTicks() - imu_ticks >= 1000 * 1000 * 5 ) {
			gDebug() << "Sampling rate : " << ( imu_fps / 5 ) << " Hz\n";
			imu_fps = 0;
			imu_ticks = Board::GetTicks();
		}
	} else {
		usleep( 1000 * 100 );
		mSensorsThreadTick = Board::GetTicks();
	}
	return true;
*/	// TEST
	return false;
}


void IMU::Loop( float dt )
{
	if ( mState == Off ) {
		// Nothing to do
	} else if ( mState == Calibrating or mState == CalibratingAll ) {
		Calibrate( dt, ( mState == CalibratingAll ) );
	} else if ( mState == CalibrationDone ) {
		mState = Running;
		mMain->blackbox()->Enqueue( "IMU:state", "Running" );
	} else if ( mState == Running ) {
		UpdateSensors( dt, ( mMain->stabilizer()->mode() == Stabilizer::Rate ) );
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
			gDebug() << "Calibrating " << ( all ? "all " : "" ) << "sensors\n";
			gDebug() << "calibrate " << mCalibrationStep << " " << 0 << "\n";
			mMain->blackbox()->Enqueue( "IMU:state", "Calibrating" );
			gDebug() << "calibrate " << mCalibrationStep << " " << 1 << "\n";
			mCalibrationStep++;
			mCalibrationTimer = Board::GetTicks();
			gDebug() << "calibrate " << mCalibrationStep << " " << 2 << "\n";
			break;
		}
		case 1 : {
			gDebug() << "calibrate " << mCalibrationStep << " " << 0 << "\n";
			for ( auto dev : Sensor::Devices() ) {
				if ( all or dynamic_cast< Gyroscope* >( dev ) != nullptr ) {
					gDebug() << "calibrate " << dev->names().front();
					dev->Calibrate( dt, false );
				}
			}
			gDebug() << "calibrate " << mCalibrationStep << " " << 1 << "\n";
			if ( Board::GetTicks() - mCalibrationTimer >= 1000 * 1000 * 2 ) {
				mCalibrationStep++;
			}
			break;
		}
		case 2 : {
			gDebug() << "Calibration last pass\n";
			for ( auto dev : Sensor::Devices() ) {
				if ( all or dynamic_cast< Gyroscope* >( dev ) != nullptr ) {
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
			gDebug() << "Calibrating gravity\n";
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
			gDebug() << "Calibration almost done...\n";
			mState = CalibrationDone;
			mMain->blackbox()->Enqueue( "IMU:state", "CalibrationDone" );
			mAcceleration = Vector3f();
			mGyroscope = Vector3f();
			mMagnetometer = Vector3f();
			mRPY = Vector3f();
			mdRPY = Vector3f();
			mRate = Vector3f();
			gDebug() << "Calibration done !\n";
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
		gDebug() << "Full recalibration needed...\n";
		RecalibrateAll();
		return;
	}

	mCalibrationStep = 0;
	mState = Calibrating;
	mRPYOffset = Vector3f();
	gDebug() << "Calibrating gyroscope...\n";
}


void IMU::RecalibrateAll()
{
	mCalibrationStep = 0;
	mState = CalibratingAll;
	mRPYOffset = Vector3f();
	gDebug() << "Calibrating all sensors...\n";
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

void geNormalize( Vector3f* v )
{
	double l = sqrt((double)( (v->x*v->x) + (v->y*v->y) + (v->z*v->z) ));
	if ( l > 0.00001f ) {
		double il = 1.0f / l;
		v->x *= il;
		v->y *= il;
		v->z *= il;
	}
}


void IMU::UpdateSensors( float dt, bool gyro_only )
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

	for ( Gyroscope* dev : Sensor::Gyroscopes() ) {
		vtmp.x = vtmp.y = vtmp.z = 0.0f;
		int ret = dev->Read( &vtmp );
		if ( ret > 0 ) {
			total_gyro += Vector4f( vtmp, 1.0f );
		}
	}
	if ( total_gyro.w > 0.0f ) {
		mGyroscope = total_gyro.xyz() / total_gyro.w;
	}
	sprintf( stmp, "\"%.4f,%.4f,%.4f\"", mGyroscope.x, mGyroscope.y, mGyroscope.z );
	mMain->blackbox()->Enqueue( "IMU:gyroscope", stmp );

	if ( mState == Running and ( not gyro_only or mAcroRPYCounter == 0 ) )
	{
		for ( Accelerometer* dev : Sensor::Accelerometers() ) {
			vtmp.x = vtmp.y = vtmp.z = 0.0f;
			dev->Read( &vtmp );
			total_accel += Vector4f( vtmp, 1.0f );
		}
		if ( total_accel.w > 0.0f ) {
			mLastAcceleration = mAcceleration;
			mAcceleration = total_accel.xyz() / total_accel.w;
		}
// 		geNormalize( &mAcceleration );
		sprintf( stmp, "\"%.4f,%.4f,%.4f\"", mAcceleration.x, mAcceleration.y, mAcceleration.z );
		mMain->blackbox()->Enqueue( "IMU:acceleration", stmp );

		if ( mSensorsUpdateSlow % 8 == 0 ) {
			for ( Magnetometer* dev : Sensor::Magnetometers() ) {
				vtmp.x = vtmp.y = vtmp.z = 0.0f;
				dev->Read( &vtmp );
				total_magn += Vector4f( vtmp, 1.0f );
			}
			if ( total_magn.w > 0.0f ) {
				mMagnetometer = total_magn.xyz() / total_magn.w;
			}
			sprintf( stmp, "\"%.4f,%.4f,%.4f\"", mMagnetometer.x, mMagnetometer.y, mMagnetometer.z );
			mMain->blackbox()->Enqueue( "IMU:magnetometer", stmp );
		}

		if ( mSensorsUpdateSlow % 16 == 0 ) {
			for ( Altimeter* dev : Sensor::Altimeters() ) {
				ftmp = 0.0f;
				dev->Read( &ftmp );
				if ( dev->type() == Altimeter::Proximity and ftmp > 0.0f ) {
					total_proxi += Vector2f( ftmp, 1.0f );
				} else if ( dev->type() == Altimeter::Absolute ) {
					total_alti += Vector2f( ftmp, 1.0f );
				}
			}
			for ( GPS* dev : Sensor::GPSes() ) {
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

	// Update RPY only at 1/32 update frequency when in Rate mode
	mAcroRPYCounter = ( mAcroRPYCounter + 1 ) % 32;
	mSensorsUpdateSlow = ( mSensorsUpdateSlow + 1 ) % 2048;
}


void IMU::UpdateAttitude( float dt )
{
	// Process rates Extended-Kalman-Filter
	mRates.UpdateInput( 0, mGyroscope.x );
	mRates.UpdateInput( 1, mGyroscope.y );
	mRates.UpdateInput( 2, mGyroscope.z );
	mRates.Process( dt );
	mRate = mRates.state( 0 );

	// Process acceleration Extended-Kalman-Filter
	mAccelerationSmoother.UpdateInput( 0, mAcceleration.x );
	mAccelerationSmoother.UpdateInput( 1, mAcceleration.y );
	mAccelerationSmoother.UpdateInput( 2, mAcceleration.z );
	mAccelerationSmoother.Process( dt );
	Vector3f accel = mAccelerationSmoother.state( 0 );

	Vector2f accel_roll_pitch = Vector2f();
	if ( abs( accel.x ) >= 0.5f * 9.8f or abs( accel.z ) >= 0.5f * 9.8f ) {
		accel_roll_pitch.x = atan2( accel.x, accel.z ) * 180.0f / M_PI;
	}
	if ( abs( accel.y ) >= 0.5f * 9.8f or abs( accel.z ) >= 0.5f * 9.8f ) {
		accel_roll_pitch.y = atan2( accel.y, accel.z ) * 180.0f / M_PI;
	}

	// Update accelerometer values
	mAttitude.UpdateInput( 0, accel_roll_pitch.x );
	mAttitude.UpdateInput( 1, accel_roll_pitch.y );
	if ( mMain->stabilizer()->mode() != Stabilizer::Rate and 0 /*TODO*/ ) {
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

	char tmp[64];
	sprintf( tmp, "\"%.4f,%.4f,%.4f\"", mRate.x, mRate.y, mRate.z );
	mMain->blackbox()->Enqueue( "IMU:rate", tmp );
	sprintf( tmp, "\"%.4f,%.4f,%.4f\"", mRPY.x, mRPY.y, mRPY.z );
	mMain->blackbox()->Enqueue( "IMU:rpy", tmp );
}


void IMU::UpdateVelocity( float dt )
{
	Vector3f velo = mVelocity.state( 0 );
	Vector3f accel = mAccelerationSmoother.state( 0 );

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
