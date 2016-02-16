#include <unistd.h>
#include <algorithm>
#include "Main.h"
#include "Controller.h"
#include <Link.h>
#include <IMU.h>
#include <Gyroscope.h>
#include <Accelerometer.h>
#include <Magnetometer.h>
#include <Sensor.h>
#include <Stabilizer.h>
#include <Frame.h>
#include <Servo.h>

#include <netinet/in.h>

Controller::Controller( Main* main, Link* link )
	: Thread( "controller" )
	, mMain( main )
	, mLink( link )
	, mConnected( false )
	, mArmed( false )
	, mRPY( Vector3f() )
	, mThrust( 0.0f )
	, mTicks( 0 )
{
	Start();
	while ( !mLink->isConnected() ) {
		usleep( 1000 * 10 );
	}
}


Controller::~Controller()
{
}


bool Controller::armed() const
{
	return mArmed;
}


float Controller::thrust() const
{
	return mThrust;
}


const Vector3f& Controller::RPY() const
{
// 	return mSmoothRPY;
	return mRPY;
}


void Controller::Emergency()
{
	mMain->stabilizer()->Reset( 0.0f );
	mMain->frame()->Disarm();
	Servo::HardwareSync();
	mThrust = 0.0f;
	mSmoothRPY = Vector3f();
	mRPY = Vector3f();
	mMain->stabilizer()->Reset( 0.0f );
	mMain->frame()->Disarm();
	Servo::HardwareSync();
}


void Controller::UpdateSmoothControl( const float& dt )
{
// 	mSmoothControl.Predict( dt );
// 	mSmoothRPY = mSmoothControl.Update( dt, mRPY );
}


bool Controller::run()
{
	uint32_t raw_cmd;
	Cmd cmd;

	mConnected = mLink->isConnected();
	if ( !mConnected ) {
		mRPY.x = 0.0f;
		mRPY.y = 0.0f;
	}

	if ( !mLink->isConnected() ) {
		mConnected = false;
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Controller connected !\n";
		}
		return true;
	}

	if ( mLink->ReadU32( &raw_cmd ) <= 0 ) {
		gDebug() << "Controller connection lost !\n";
		return true;
	}
	cmd = (Cmd)raw_cmd;

// 	gDebug() << "Received cmd : " << (int)buf[0] << "\n";

	switch ( cmd )
	{
		case PING : {
			uint32_t ticks = 0;
			if ( mLink->ReadU32( &ticks ) == sizeof(uint32_t) ) {
				mLink->WriteU32( ticks );
			}
			break;
		}
		case CALIBRATE : {
			uint32_t full_recalibration = 0;
			float curr_altitude = 0.0f;
			if ( mLink->ReadU32( &full_recalibration ) == sizeof(uint32_t) and mLink->ReadFloat( &curr_altitude ) == sizeof(float) ) {
				if ( mArmed ) {
					mLink->WriteU32( 0xFFFFFFFF );
				} else {
					if ( full_recalibration ) {
						mMain->imu()->RecalibrateAll();
					} else {
						mMain->imu()->Recalibrate();
					}
					mLink->WriteU32( 0 );
				}
			}
			break;
		}
		case SET_TIMESTAMP : {
			uint32_t timestamp = 0;
			if ( mLink->ReadU32( &timestamp ) == sizeof(timestamp) ) {
				mMain->board()->setLocalTimestamp( timestamp );
				mLink->WriteU32( 0 );
			}
			break;
		}
		case ARM : {
			mArmed = true;
// 			mMain->imu()->ResetYaw();
			mMain->stabilizer()->Reset( mMain->imu()->RPY().z );
			mMain->frame()->Arm();
			mRPY.z = mMain->imu()->RPY().z;
			gDebug() << "RPY.z : " << mRPY.z << "\n";
			mLink->WriteU32( mArmed );
			break;
		}
		case DISARM : {
			mThrust = 0.0f;
			mMain->stabilizer()->Reset( 0.0f );
			mMain->frame()->Disarm();
			mRPY = Vector3f();
// 			mSmoothControl.setState( Vector3f() );
			mSmoothRPY = Vector3f();
			mArmed = false;
			mLink->WriteU32( mArmed );
			break;
		}
		case RESET_BATTERY : {
			uint32_t value = 0;
			if ( mLink->ReadU32( &value ) == sizeof(value) ) {
				mMain->powerThread()->ResetFullBattery( value );
				mLink->WriteU32( value );
			}
			break;
		}
		case VBAT : {
			mLink->WriteFloat( mMain->powerThread()->VBat() );
			break;
		}
		case TOTAL_CURRENT : {
			mLink->WriteFloat( mMain->powerThread()->CurrentTotal() );
			break;
		}
		case CURRENT_DRAW : {
			mLink->WriteFloat( mMain->powerThread()->CurrentDraw() );
			break;
		}
		case BATTERY_LEVEL : {
			mLink->WriteFloat( mMain->powerThread()->BatteryLevel() );
			break;
		}
		case ROLL_PITCH_YAW : {
			mLink->WriteFloat( mMain->imu()->RPY().x );
			mLink->WriteFloat( mMain->imu()->RPY().y );
			mLink->WriteFloat( mMain->imu()->RPY().z );
			break;
		}
		case SET_ROLL : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mRPY.x = value;
			mLink->WriteFloat( mRPY.x );
			break;
		}
		case SET_PITCH : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mRPY.y = value;
			mLink->WriteFloat( mRPY.y );
			break;
		}

		case SET_YAW : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mRPY.z += value;
			mLink->WriteFloat( mRPY.z );
			break;
		}

		case SET_THRUST : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mThrust = value;
			mLink->WriteFloat( mThrust );
			break;
		}
		case RESET_MOTORS : {
			mThrust = 0.0f;
			mMain->stabilizer()->Reset( 0.0f );
			mMain->frame()->Disarm();
			mRPY = Vector3f();
			mLink->WriteU32( 0 );
			break;
		}

		case SET_MODE : {
			uint32_t mode = 0;
			if( mLink->ReadU32( &mode ) == sizeof(uint32_t) ) {
				mMain->stabilizer()->setMode( mode );
			}
			break;
		}

		case SET_PID_P : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->stabilizer()->setP( value );
			mLink->WriteFloat( value );
			break;
		}
		case SET_PID_I : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->stabilizer()->setI( value );
			mLink->WriteFloat( value );
			break;
		}
		case SET_PID_D : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->stabilizer()->setD( value );
			mLink->WriteFloat( value );
			break;
		}
		case PID_FACTORS : {
			Vector3f pid = mMain->stabilizer()->getPID();
			mLink->WriteFloat( pid.x );
			mLink->WriteFloat( pid.y );
			mLink->WriteFloat( pid.z );
			break;
		}
		case PID_OUTPUT : {
			Vector3f pid = mMain->stabilizer()->lastPIDOutput();
			mLink->WriteFloat( pid.x );
			mLink->WriteFloat( pid.y );
			mLink->WriteFloat( pid.z );
			break;
		}

		case SET_OUTER_PID_P : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->stabilizer()->setOuterP( value );
			mLink->WriteFloat( value );
			break;
		}
		case SET_OUTER_PID_I : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->stabilizer()->setOuterI( value );
			mLink->WriteFloat( value );
			break;
		}
		case SET_OUTER_PID_D : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->stabilizer()->setOuterD( value );
			mLink->WriteFloat( value );
			break;
		}
		case OUTER_PID_FACTORS : {
			Vector3f pid = mMain->stabilizer()->getOuterPID();
			mLink->WriteFloat( pid.x );
			mLink->WriteFloat( pid.y );
			mLink->WriteFloat( pid.z );
			break;
		}
		case OUTER_PID_OUTPUT : {
			Vector3f pid = mMain->stabilizer()->lastOuterPIDOutput();
			mLink->WriteFloat( pid.x );
			mLink->WriteFloat( pid.y );
			mLink->WriteFloat( pid.z );
			break;
		}

		default: {
			break;
		}
	}

	return true;
}
