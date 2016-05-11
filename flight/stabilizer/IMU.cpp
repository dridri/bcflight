#include <unistd.h>
#include <cmath>
#include <Debug.h>
#include "IMU.h"
#include "Stabilizer.h"
#include "Accelerometer.h"
#include "Gyroscope.h"
#include "Magnetometer.h"
#include <Controller.h>

IMU::IMU( Main* main )
	: mMain( main )
	, mState( Off )
	, mAcceleration( Vector3f() )
	, mGyroscope( Vector3f() )
	, mMagnetometer( Vector3f() )
	, mRPY( Vector3f() )
	, mdRPY( Vector3f() )
	, mRate( Vector3f() )
	, mRPYOffset( Vector3f() )
	, mCalibrationAccum( 0 )
	, mRPYAccum( Vector4f() )
	, mRates( 3, 3 )
// 	, mAttitude( 3, 3 )
	, mAttitude( 6, 3 )
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
	mRates.setInputFilter( 0, 80.0f );
	mRates.setInputFilter( 1, 80.0f );
	mRates.setInputFilter( 2, 80.0f );

	// Set output rates filtering factors
	mRates.setOutputFilter( 0, 0.5f );
	mRates.setOutputFilter( 1, 0.5f );
	mRates.setOutputFilter( 2, 0.5f );

	/** mAttitude matrix :
	 *   - Inputs :
	 *     - accelerometer 0 1 2
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

	// Set accelerometer filtering factors
	mAttitude.setInputFilter( 0, 200.0f );
	mAttitude.setInputFilter( 1, 200.0f );
	mAttitude.setInputFilter( 2, 500.0f );

	mAttitude.setInputFilter( 3, 0.25f );
	mAttitude.setInputFilter( 4, 0.25f );
	mAttitude.setInputFilter( 5, 0.25f );

	// Set output roll-pitch-yaw filtering factors
	mAttitude.setOutputFilter( 0, 0.5f );
	mAttitude.setOutputFilter( 1, 0.5f );
	mAttitude.setOutputFilter( 2, 0.5f );

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
		bool rate = ( mMain->stabilizer()->mode() == Stabilizer::Rate );
		float dt = ((float)( Board::GetTicks() - mSensorsThreadTick ) ) / 1000000.0f;
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
		UpdateRPY( dt );
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
	Vector3f vtmp;

	for ( Gyroscope* dev : Sensor::Gyroscopes() ) {
		dev->Read( &vtmp );
		total_gyro += Vector4f( vtmp, 1.0f );
	}
	mGyroscope = total_gyro.xyz() / total_gyro.w;

	if ( mState == Running and ( not gyro_only /*or mAcroRPYCounter == 0*/ ) ) {
		for ( Accelerometer* dev : Sensor::Accelerometers() ) {
			dev->Read( &vtmp );
			total_accel += Vector4f( vtmp, 1.0f );
		}
		/*
		for ( Magnetometer* dev : Sensor::Magnetometers() ) {
			dev->Read( &vtmp );
			total_magn += Vector4f( vtmp, 1.0f );
		}
		*/
		mLastAcceleration = mAcceleration;
		mAcceleration = total_accel.xyz() / total_accel.w;
		mMagnetometer = total_magn.xyz() / total_magn.w;
	}
}

static uint32_t dump = 0;


void IMU::UpdateRPY( float dt )
{
	// Process rates Extended-Kalman-Filter
	mRates.UpdateInput( 0, mGyroscope.x );
	mRates.UpdateInput( 1, mGyroscope.y );
	mRates.UpdateInput( 2, mGyroscope.z );
	mRates.Process( dt );
	mRate = mRates.state( 0 );

	Vector2f accel_rp = Vector2f();

	if ( std::abs( mAcceleration[0] ) >= 0.5f * 9.8f or std::abs( mAcceleration[2] ) >= 0.5f * 9.8f ) {
		accel_rp.x = std::atan2( mAcceleration[0], mAcceleration[2] ) * 180.0f / M_PI;
	}
	if ( std::abs( mAcceleration[1] ) >= 0.5f * 9.8f or std::abs( mAcceleration[2] ) >= 0.5f * 9.8f ) {
		accel_rp.y = std::atan2( mAcceleration[1], mAcceleration[2] ) * 180.0f / M_PI;
	}

	Vector3f gyro = mRate;
// 	Vector3f gyro = mGyroscope;

	float magn_yaw = mRPY.z + gyro.z * dt; // TODO : calculate using magnetometer, for now, use integrated value from gyro

	// Update accelerometer values
	mAttitude.UpdateInput( 0, accel_rp.x );
	mAttitude.UpdateInput( 1, accel_rp.y );
	mAttitude.UpdateInput( 2, magn_yaw );

	// Integrate and update rates values
	mAttitude.UpdateInput( 3, mRPY.x + gyro.x * dt );
	mAttitude.UpdateInput( 4, mRPY.y + gyro.y * dt );
	mAttitude.UpdateInput( 5, mRPY.z + gyro.z * dt );

	// Process Extended-Kalman-Filter
	mAttitude.Process( dt );

	// Retrieve results
	Vector4f rpy = mAttitude.state( 0 );
	mdRPY = ( rpy - mRPY ) * dt;
	mRPY = rpy;
/*
	if ( dump++ >= 10 ) {
		mAttitude.DumpInput();
		printf( " => { %.4f, %.4f, %.4f }\n", mRPY.x, mRPY.y, mRPY.z );
		printf( " magn_yaw : %.4f\n", magn_yaw );
		printf( " gyro { %.4f, %.4f, %.4f }\n", gyro.x, gyro.y, gyro.z );
		printf( " rate { %.4f, %.4f, %.4f }\n", mRate.x, mRate.y, mRate.z );
		printf( "\n" );
		dump = 0;
	}
*/
}
