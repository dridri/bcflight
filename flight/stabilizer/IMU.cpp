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
#include "Stabilizer.h"
#include "Accelerometer.h"
#include "Gyroscope.h"
#include "Magnetometer.h"
#include "Altimeter.h"
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
	, mCalibrationAccum( 0 )
	, mRPYAccum( Vector4f() )
	, mGravity( Vector3f() )
	, mRates( 3, 3 )
	, mAccelerationSmoother( 3, 3 )
// 	, mAttitude( 3, 3 )
	, mAttitude( 6, 3 )
	, mPosition( 1, 1 )
	, mLastAccelAttitude( Vector4f() )
	, mLastAcceleration( Vector3f() )
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
	mRates.setInputFilter( 0, main->config()->number( "stabilizer.filters.rates.input.x", 80.0f ) );
	mRates.setInputFilter( 1, main->config()->number( "stabilizer.filters.rates.input.y", 80.0f ) );
	mRates.setInputFilter( 2, main->config()->number( "stabilizer.filters.rates.input.z", 80.0f ) );

	// Set output rates filtering factors
	mRates.setOutputFilter( 0, main->config()->number( "stabilizer.filters.rates.output.x", 0.5f ) );
	mRates.setOutputFilter( 1, main->config()->number( "stabilizer.filters.rates.output.y", 0.5f ) );
	mRates.setOutputFilter( 2, main->config()->number( "stabilizer.filters.rates.output.z", 0.5f ) );

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
	mAccelerationSmoother.setInputFilter( 0, main->config()->number( "stabilizer.filters.accelerometer.input.x", 100.0f ) );
	mAccelerationSmoother.setInputFilter( 1, main->config()->number( "stabilizer.filters.accelerometer.input.y", 100.0f ) );
	mAccelerationSmoother.setInputFilter( 2, main->config()->number( "stabilizer.filters.accelerometer.input.z", 250.0f ) );

	// Set output rates filtering factors
	mAccelerationSmoother.setOutputFilter( 0, main->config()->number( "stabilizer.filters.accelerometer.output.x", 0.5f ) );
	mAccelerationSmoother.setOutputFilter( 1, main->config()->number( "stabilizer.filters.accelerometer.output.y", 0.5f ) );
	mAccelerationSmoother.setOutputFilter( 2, main->config()->number( "stabilizer.filters.accelerometer.output.z", 0.5f ) );

	/** mAttitude matrix :
	 *   - Inputs :
	 *     - smoothed linear acceleration 0 1 2
	 *     - rates 3 4 5
	 *   - Outputs :
	 *     - roll-pitch-yaw 0 1 2
	 **/
	// Set Extended-Kalman-Filter mixing matrix (input, output, factor)
	mAttitude.setSelector( 0, 0, 1.0f );
	mAttitude.setSelector( 3, 0, 1.0f );
	mAttitude.setSelector( 1, 1, 1.0f );
	mAttitude.setSelector( 4, 1, 1.0f );
	mAttitude.setSelector( 2, 2, 1.0f );
	mAttitude.setSelector( 5, 2, 1.0f );

	// Set acceleration filtering factors
	mAttitude.setInputFilter( 0, main->config()->number( "stabilizer.filters.attitude.input.accelerometer.x", 0.1f ) );
	mAttitude.setInputFilter( 1, main->config()->number( "stabilizer.filters.attitude.input.accelerometer.y", 0.1f ) );
	mAttitude.setInputFilter( 2, main->config()->number( "stabilizer.filters.attitude.input.accelerometer.z", 0.1f ) );

	// Set rates filtering factors
	mAttitude.setInputFilter( 3, main->config()->number( "stabilizer.filters.attitude.input.rates.x", 0.01f ) );
	mAttitude.setInputFilter( 4, main->config()->number( "stabilizer.filters.attitude.input.rates.y", 0.01f ) );
	mAttitude.setInputFilter( 5, main->config()->number( "stabilizer.filters.attitude.input.rates.z", 0.01f ) );

	// Set output roll-pitch-yaw filtering factors
	mAttitude.setOutputFilter( 0, main->config()->number( "stabilizer.filters.attitude.output.z", 0.25f ) );
	mAttitude.setOutputFilter( 1, main->config()->number( "stabilizer.filters.attitude.output.z", 0.25f ) );
	mAttitude.setOutputFilter( 2, main->config()->number( "stabilizer.filters.attitude.output.z", 0.25f ) );

	/** mPosition matrix :
	 *   - Inputs :
	 *     - altitude (either absolute or proximity) 0
	 *   - Outputs :
	 *     - smoothed altitude 0
	 **/
	mPosition.setSelector( 0, 0, 1.0f );
	mPosition.setInputFilter( 0, 10.0f );
	mPosition.setOutputFilter( 0, 0.25f );

	mSensorsThreadTick = 0;
	mSensorsThreadTickRate = 0;
	mSensorsThread = new HookThread<IMU>( "imu_sensors", this, &IMU::SensorsThreadRun );
	mSensorsThread->Start();
	mSensorsThread->setPriority( 99 );
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


static int imu_fps = 0;
static uint64_t imu_ticks = 0;
bool IMU::SensorsThreadRun()
{
	if ( mState == Running ) {
		float dt = ((float)( Board::GetTicks() - mSensorsThreadTick ) ) / 1000000.0f;
		mSensorsThreadTick = Board::GetTicks();
		if ( std::abs( dt ) >= 1.0 ) {
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
}


void IMU::Loop( float dt )
{
	if ( mState == Off ) {
		// Nothing to do
	} else if ( mState == Calibrating or mState == CalibratingAll ) {
		Calibrate( dt, ( mState == CalibratingAll ) );
	} else if ( mState == CalibrationDone ) {
		mState = Running;
	} else if ( mState == Running ) {
		UpdateAttitude( dt );
		if ( mPositionUpdate ) {
			mPositionUpdateMutex.lock();
			mPositionUpdate = false;
			mPositionUpdateMutex.unlock();
			UpdatePosition( dt );
		}
	}
}


void IMU::Calibrate( float dt, bool all )
{
	if ( mCalibrationAccum < 2000 ) {
		if ( mCalibrationAccum == 0 ) {
			gDebug() << "Calibrating " << ( all ? "all " : "" ) << "sensors\n";
		}
		bool last_pass = ( mCalibrationAccum >= 1999 );
		if ( last_pass ) {
			gDebug() << "Calibration last pass\n";
		}
		for ( auto dev : Sensor::Devices() ) {
			if ( all or dynamic_cast< Gyroscope* >( dev ) != nullptr ) {
				dev->Calibrate( dt, last_pass );
			}
		}
	} else if ( all and mCalibrationAccum < 3000 ) {
		if ( mCalibrationAccum == 2000 ) {
			gDebug() << "Calibrating gravity\n";
		}
		UpdateSensors( dt );
		mGravity += mAcceleration / 1000.0f;
	} else {
		gDebug() << "Calibration almost done...\n";
		mState = CalibrationDone;
		mAcceleration = Vector3f();
		mGyroscope = Vector3f();
		mMagnetometer = Vector3f();
		mRPY = Vector3f();
		mdRPY = Vector3f();
		mRate = Vector3f();
		gDebug() << "Calibration done !\n";
	}

	mCalibrationAccum++;
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
		RecalibrateAll();
		return;
	}

	mCalibrationAccum = 0;
	mState = Calibrating;
	mRPYOffset = Vector3f();
	gDebug() << "Calibrating...\n";
}


void IMU::RecalibrateAll()
{
	mCalibrationAccum = 0;
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


void IMU::UpdateSensors( float dt, bool gyro_only )
{
	Vector4f total_accel;
	Vector4f total_gyro;
	Vector4f total_magn;
	Vector2f total_alti;
	Vector2f total_proxi;
	Vector3f vtmp;
	float ftmp;

	for ( Gyroscope* dev : Sensor::Gyroscopes() ) {
		dev->Read( &vtmp );
		total_gyro += Vector4f( vtmp, 1.0f );
	}
	mGyroscope = total_gyro.xyz() / total_gyro.w;

	if ( mState == Running and ( not gyro_only /*or mAcroRPYCounter == 0*/ ) )
	{
		for ( Accelerometer* dev : Sensor::Accelerometers() ) {
			dev->Read( &vtmp );
			total_accel += Vector4f( vtmp, 1.0f );
		}
		mLastAcceleration = mAcceleration;
		mAcceleration = total_accel.xyz() / total_accel.w;

		if ( mSensorsUpdateSlow % 16 == 0 ) {
			for ( Magnetometer* dev : Sensor::Magnetometers() ) {
				dev->Read( &vtmp );
				total_magn += Vector4f( vtmp, 1.0f );
			}
			mMagnetometer = total_magn.xyz() / total_magn.w;
		}

		if ( mSensorsUpdateSlow % 32 == 0 ) {
			for ( Altimeter* dev : Sensor::Altimeters() ) {
				dev->Read( &ftmp );
				if ( dev->type() == Altimeter::Proximity and ftmp > 0.0f ) {
					total_proxi += Vector2f( ftmp, 1.0f );
				} else if ( dev->type() == Altimeter::Absolute ) {
					total_alti += Vector2f( ftmp, 1.0f );
				}
			}
			if ( total_alti.y > 0.0f ) {
				mAltitude = total_alti.x / total_alti.y;
			} else {
				mAltitude = 0.0f;
			}
			if ( total_proxi.y > 0.0f ) {
				mProximity = total_proxi.x / total_proxi.y;
			} else {
				mProximity = 0.0f;
			}

			mPositionUpdateMutex.lock();
			mPositionUpdate = true;
			mPositionUpdateMutex.unlock();
		}

		mSensorsUpdateSlow = ( mSensorsUpdateSlow + 1 ) % 2048;
	}
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

	Vector2f accel_rp = Vector2f();
	if ( std::abs( accel.x ) >= 0.5f * 9.8f or std::abs( accel.z ) >= 0.5f * 9.8f ) {
		accel_rp.x = std::atan2( accel.x, accel.z ) * 180.0f / M_PI;
	}
	if ( std::abs( accel.y ) >= 0.5f * 9.8f or std::abs( accel.z ) >= 0.5f * 9.8f ) {
		accel_rp.y = std::atan2( accel.y, accel.z ) * 180.0f / M_PI;
	}

	// Update accelerometer values
	mAttitude.UpdateInput( 0, accel_rp.x );
	mAttitude.UpdateInput( 1, accel_rp.y );
	mAttitude.UpdateInput( 2, mRPY.z + mRate.z * dt ); // TODO : calculate using magnetometer, for now, use integrated value from gyro

	// Integrate and update rates values
	mAttitude.UpdateInput( 3, mRPY.x + mRate.x * dt );
	mAttitude.UpdateInput( 4, mRPY.y + mRate.y * dt );
	mAttitude.UpdateInput( 5, mRPY.z + mRate.z * dt );

	// Process Extended-Kalman-Filter
	mAttitude.Process( dt );

	// Retrieve results
	Vector4f rpy = mAttitude.state( 0 );
	mdRPY = ( rpy - mRPY ) * dt;
	mRPY = rpy;
}


void IMU::UpdatePosition( float dt )
{
	float altitude = 0.0f;

	if ( mProximity > 0.0f ) {
		mAltitudeOffset = mAltitude - mProximity;
		altitude = mProximity;
	} else {
		altitude = mAltitude - mAltitudeOffset;
	}
	mPosition.UpdateInput( 0, altitude );

/*
	// Calculate acceleration on Z-axis
	float accel_z = std::cos( mRPY.x ) * std::cos( mRPY.y ) * ( mAcceleration.z - mGravity.z );
	// Integrate position on Z-axis
	float pos_z = mPosition.state( 0 ) + accel_z * dt * dt;
	mPosition.UpdateInput( 1, pos_z );
*/

	mPosition.Process( dt );
}
