#include <unistd.h>
#include <cmath>
#include <Debug.h>
#include "IMU.h"
#include "Accelerometer.h"
#include "Gyroscope.h"
#include "Magnetometer.h"

IMU::IMU()
	: mState( Off )
	, mAcceleration( Vector3f() )
	, mGyroscope( Vector3f() )
	, mMagnetometer( Vector3f() )
	, mRPY( Vector3f() )
	, mdRPY( Vector3f() )
	, mRate( Vector3f() )
	, mRPYOffset( Vector3f() )
	, mCalibrationAccum( 0 )
	, mRPYAccum( Vector4f() )
	, mSmoothAcceleration( EKFSmoother( 3, Vector4f( 0.1f, 0.1f, 0.1f ), Vector4f( 30.0f, 30.0f, 300.0f ) ) )
	, mSmoothGyroscope( EKFSmoother( 3, Vector4f( 1.0f, 1.0f, 1.0f ), Vector4f( 0.1f, 0.1f, 1.0f ) ) )
	, mSmoothMagnetometer( EKFSmoother( 3, Vector4f( 0.1f, 0.1f, 0.1f ), Vector4f( 25.0f, 25.0f, 25.0f ) ) )
	, mAttitude( EKF() )
	, mLastAccelAttitude( Vector4f() )
	, mLastAcceleration( Vector3f() )
{
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
// 	aDebug( "mRPY", mRPY.x, mRPY.y, mRPY.z );
// 	aDebug( "mRPYOffset", mRPYOffset.x, mRPYOffset.y, mRPYOffset.z );
// 	aDebug( "mdRPY", mdRPY.x, mdRPY.y, mdRPY.z );
	return mRPY;// - mRPYOffset;
}


const Vector3f IMU::dRPY() const
{
	return mdRPY;
}


const Vector3f IMU::rate() const
{
	return mRate;
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


bool IMU::SensorsThreadRun()
{
	if ( mState == Running ) {
		float dt = ((float)( Board::GetTicks() - mSensorsThreadTick ) ) / 1000000.0f;
		UpdateSensors( dt );
		// Limit update rate to 200Hz since sensors/I2C driver won't keep up at faster rates
		mSensorsThreadTickRate = Board::WaitTick( 1000000 / 200, mSensorsThreadTickRate, -100 );
	} else {
		usleep( 1000 * 100 );
	}
	return true;
}


void IMU::Loop( float dt, bool update_rpy )
{
	if ( mState == Off ) {
		// Nothing to do
	} else if ( mState == Calibrating or mState == CalibratingAll ) {
		Calibrate( dt, ( mState == CalibratingAll ) );
	} else if ( mState == CalibrationDone ) {
		mState = Running;
	} else if ( mState == Running ) {
// 		UpdateSensors( dt );
		if ( update_rpy ) {
			UpdateRPY( dt );
		}
	}
}


void IMU::Calibrate( float dt, bool all )
{
	if ( mCalibrationAccum < 2000 ) {
		bool last_pass = ( mCalibrationAccum >= 1999 );
		for ( auto dev : Sensor::Devices() ) {
			if ( all or dynamic_cast< Accelerometer* >( dev ) == nullptr ) {
				dev->Calibrate( dt, last_pass );
			}
		}
	} else {
		mState = CalibrationDone;
		gDebug() << "Calibration done !\n";
/*
		UpdateSensors( dt );
		UpdateRPY( dt );
		if ( mdRPY.length() < 0.0025f or ( mRPYAccum.x != 0.0f or mRPYAccum.y != 0.0f or mRPYAccum.z != 0.0f ) ) {
			Vector4f RPYAccum = mRPYAccum + Vector4f( mRPY, 1.0f );
			mdRPYAccum = RPYAccum / RPYAccum.w - mRPYAccum / mRPYAccum.w;
			mRPYAccum = RPYAccum;
		}

		float epsilon = 0.001f;
		if ( mRPYAccum.xyz().length() > 0.0f and fabsf( mdRPYAccum.x ) < epsilon and fabsf( mdRPYAccum.y ) < epsilon ) {
			mRPYOffset = mRPYAccum.xyz() / mRPYAccum.w;
			mState = CalibrationDone;
			gDebug() << "Calibration done !\n";
			aDebug( "Roll-Pitch-Yaw Offset", mRPYOffset.x, mRPYOffset.y, mRPYOffset.z );
		}
*/
// 		aDebug( "mRPY", mRPY.x, mRPY.y, mRPY.z );
// 		aDebug( "mRPYAccum", mRPYAccum.x / mRPYAccum.w, mRPYAccum.y / mRPYAccum.w, mRPYAccum.z / mRPYAccum.w, mRPYAccum.w );
// 		aDebug( "mdRPYAccum", mdRPYAccum.x, mdRPYAccum.y, mdRPYAccum.z );
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
	if (cal_all ) {
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


void IMU::ResetYaw()
{
	mVirtualNorth = Vector3f();
	while ( mVirtualNorth.x == 0.0f and mVirtualNorth.y == 0.0f and mVirtualNorth.z == 0.0f ) {
		usleep( 1000 );
	}
}


void IMU::UpdateSensors( float dt )
{
	Vector4f total_accel;
	Vector4f total_gyro;
	Vector4f total_magn;
	Vector3f vtmp;

	for ( Accelerometer* dev : Sensor::Accelerometers() ) {
		dev->Read( &vtmp );
		total_accel += Vector4f( vtmp, 1.0f );
	}

	for ( Gyroscope* dev : Sensor::Gyroscopes() ) {
		dev->Read( &vtmp );
		total_gyro += Vector4f( vtmp, 1.0f );
	}

	for ( Magnetometer* dev : Sensor::Magnetometers() ) {
		dev->Read( &vtmp );
		total_magn += Vector4f( vtmp, 1.0f );
	}

	mLastAcceleration = mAcceleration;

	if ( mCalibrationAccum == 2000 ) {
		mSmoothAcceleration.setState( Vector4f( 0.0f, 0.0f, total_accel.z / total_accel.w, 0.0f ) );
		mSmoothMagnetometer.setState( total_magn.xyz() / total_magn.w );
	} else {
		mSmoothAcceleration.Predict( dt );
		mAcceleration = mSmoothAcceleration.Update( dt, total_accel.xyz() / total_accel.w );

		mSmoothGyroscope.Predict( dt );
		mGyroscope = mSmoothGyroscope.Update( dt, total_gyro.xyz() / total_gyro.w );

		mSmoothMagnetometer.Predict( dt );
		mMagnetometer = mSmoothMagnetometer.Update( dt, total_magn.xyz() / total_magn.w );
	}
}


void IMU::UpdateRPY( float dt )
{
	Vector3f accel = Vector3f();

	if ( fabsf( mAcceleration[0] ) >= 0.5f * 9.8f or fabsf( mAcceleration[2] ) >= 0.5f * 9.8f ) {
		accel.x = atan2f( mAcceleration[0], mAcceleration[2] ) * 180.0f / M_PI;
	}
	if ( fabsf( mAcceleration[1] ) >= 0.5f * 9.8f or fabsf( mAcceleration[2] ) >= 0.5f * 9.8f ) {
		accel.y = atan2f( mAcceleration[1], mAcceleration[2] ) * 180.0f / M_PI;
	}

	Vector3f north = mMagnetometer.xyz();
	north.normalize();
	north.w = 1.0f;

	Matrix rot_matrix;
	rot_matrix.Identity();
	rot_matrix.RotateY( -mRPY.x * M_PI / 180.0f );
	rot_matrix.RotateX( -mRPY.y * M_PI / 180.0f );
	north = ( rot_matrix * north ).xyz();
/*
	accel.z = ( atan2f( north.y, north.x ) ) * 180.0f / M_PI;
	if ( accel.z < 0.0f and fmodf( mRPY.z, 360.0f ) > 150.0f ) {
		accel.z = mRPY.z + 180.0f + accel.z;
	} else if ( accel.z > 0.0f and fmodf( mRPY.z, 360.0f ) < -150.0f ) {
		accel.z = mRPY.z - 180.0f + accel.z;
	}
*/
/*
	if ( mVirtualNorth.x == 0.0f and mVirtualNorth.y == 0.0f and mVirtualNorth.z == 0.0f ) {
		mVirtualNorth = north;
		mRPY.z = accel.z;
		return;
	}
*/
	float accel_f = 0.003f;
	float inv_accel_f = 1.0f - accel_f;
// 	float magn_f = 0.01f;
// 	float inv_magn_f = 1.0f - magn_f;

	mRate = Vector3f( -mGyroscope.y, mGyroscope.x, mGyroscope.z ) * dt;
	Vector3f integration = mRPY + mRate;
/*
	if ( ( accel.x < 0.0f and integration.x >= 140.0f ) or ( accel.x > 0.0f and integration.x <= -140.0f ) ) {
		integration.x = accel.x;
	}
	if ( ( accel.y < 0.0f and integration.y >= 140.0f ) or ( accel.y > 0.0f and integration.y <= -140.0f ) ) {
		integration.y = accel.y;
	}
*/
	Vector3f ret = Vector3f( integration.x, integration.y, integration.z );
	if ( accel.x != 0.0f ) {
		ret.x = ret.x * inv_accel_f + accel.x * accel_f;
	}
	if ( accel.y != 0.0f ) {
		ret.y = ret.y * inv_accel_f + accel.y * accel_f;
	}
	if ( accel.z != 0.0f ) {
// 		ret.z = ret.z * inv_magn_f + accel.z * magn_f;
	}

	mRPY = ret;
}
