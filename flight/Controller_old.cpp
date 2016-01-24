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
#include <Frame.h>
#include <Servo.h>

#include <netinet/in.h>

Controller::Controller( Main* main, Link* link )
	: Thread()
	, mMain( main )
	, mLink( link )
	, mRPY( Vector3f() )
	, mThrust( 0.0f )
	, mTicks( 0 )
{
	Start();
// 	while ( !mLink->isConnected() ) {
// 		usleep( 1000 * 10 );
// 	}
}


Controller::~Controller()
{
}


float Controller::thrust() const
{
	return mThrust;
}


const Vector3f& Controller::RPY() const
{
	return mRPY;
}


void Controller::Emergency()
{
	mMain->frame()->ResetMotors();
	Servo::HardwareSync();
	mThrust = 0.0f;
	mRPY = Vector3f();
	mMain->frame()->ResetMotors();
	Servo::HardwareSync();
}


bool Controller::run()
{
	uint8_t buf[128] = { 0 };

	if ( !mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Controller connected !\n";
		}
		return true;
	}

	if ( mLink->Read( buf, 1 ) <= 0 ) {
		gDebug() << "Controller connection lost !\n";
		return true;
	}

// 	gDebug() << "Received cmd : " << (int)buf[0] << "\n";

	Cmd cmd = (Cmd)buf[0];
	switch ( cmd )
	{
		case CALIBRATE : {
			float curr_altitude;
			if ( mLink->ReadFloat( &curr_altitude ) == sizeof(curr_altitude) ) {
				// TODO : Calibrate
				mLink->WriteString( "[IMU][CALIBRATE]SUCCESS" );
			}
			break;
		}
		case SET_TIMESTAMP : {
			uint32_t timestamp = 0;
			if ( mLink->ReadU32( &timestamp ) == sizeof(timestamp) ) {
				mMain->board()->setLocalTimestamp( timestamp );
				mLink->WriteString( "" );
			}
			break;
		}
		case PRESSURE : {
			mLink->WriteString( "[IMU][PRESSURE]%f", 0/*mMain->imu()->pressure()*/ );
			break;
		}
		case TEMPERATURE : {
			mLink->WriteString( "[IMU][TEMPERATURE]%f", 0/*mMain->imu()->temperature()*/ );
			break;
		}
		case ALTITUDE : {
			mLink->WriteString( "[IMU][ALTITUDE]%f", 0/*mMain->imu()->altitude()*/ );
			break;
		}
		case ROLL : {
			mLink->WriteString( "[IMU][ROLL]%f", mMain->imu()->RPY().x );
			break;
		}
		case PITCH : {
			mLink->WriteString( "[IMU][PITCH]%f", mMain->imu()->RPY().y );
			break;
		}
		case YAW : {
			mLink->WriteString( "[IMU][YAW]%f", mMain->imu()->RPY().z );
			break;
		}
		case ROLL_PITCH_YAW : {
			mLink->WriteString( "[IMU][RPY]%f,%f,%f,%f", ((float)Board::GetTicks()) / 1000000.0f, mMain->imu()->RPY().x, mMain->imu()->RPY().y, mMain->imu()->RPY().z );
			break;
		}
		case PID_OUTPUT : {
			mLink->WriteString( "[IMU][PID]%f,%f,%f,%f", ((float)Board::GetTicks()) / 1000000.0f, mMain->frame()->lastPID().x, mMain->frame()->lastPID().y, mMain->frame()->lastPID().z );
			break;
		}
		case ACCEL : {
			mLink->WriteString( "[IMU][ACCEL]%f,%f,%f,%f", ((float)Board::GetTicks()) / 1000000.0f, mMain->imu()->acceleration().x, mMain->imu()->acceleration().y, mMain->imu()->acceleration().z );
			break;
		}
		case GYRO : {
			mLink->WriteString( "[IMU][GYRO]%f,%f,%f,%f", ((float)Board::GetTicks()) / 1000000.0f, mMain->imu()->gyroscope().x, mMain->imu()->gyroscope().y, mMain->imu()->gyroscope().z );
			break;
		}
		case MAGN : {
			mLink->WriteString( "[IMU][MAGN]%f,%f,%f,%f", ((float)Board::GetTicks()) / 1000000.0f, mMain->imu()->magnetometer().x, mMain->imu()->magnetometer().y, mMain->imu()->magnetometer().z );
			break;
		}
		case MOTORS_SPEED : {
			auto motors = mMain->frame()->motors();
			std::stringstream ss;
			for ( auto motor : motors ) {
				if ( ss.str().length() > 0 ) {
					ss << ",";
				}
				ss << motor->speed();
			}
			std::string str = "[IMU][MOTORS][SPEED]" + ss.str();
			mLink->WriteString( str );
			break;
		}
		case SENSORS_DATA : {
			auto sensors = Sensor::Devices();
			std::stringstream ss;
			ss << ( ((float)Board::GetTicks()) / 1000000.0f );
			for ( auto dev : sensors ) {
				ss << "|";
				std::string type = "";
				if ( dynamic_cast< Gyroscope* >( dev ) ) {
					type = "[GYRO]";
				} else if ( dynamic_cast< Accelerometer* >( dev ) ) {
					type = "[ACCEL]";
				} else if ( dynamic_cast< Magnetometer* >( dev ) ) {
					type = "[MAG]";
				}
				std::string name = dev->names().front();
				std::transform( name.begin(), name.end(), name.begin(), ::toupper );
				ss << type << name << "," << dev->lastValues().x << "," << dev->lastValues().y << "," << dev->lastValues().z;
			}
			std::string str = "[IMU][SENSORS]" + ss.str();
			mLink->WriteString( str );
			break;
		}
		case VBAT : {
			std::stringstream ss;
			ss << "[IMU][VBAT]" << mMain->powerThread()->VBat();
			mLink->WriteString( ss.str() );
			break;
		}
		case TOTAL_CURRENT : {
			std::stringstream ss;
			ss << "[IMU][TOTAL_CURRENT]" << mMain->powerThread()->CurrentTotal();
			mLink->WriteString( ss.str() );
			break;
		}
		case CURRENT_DRAW : {
			std::stringstream ss;
			ss << "[IMU][CURRENT_DRAW]" << mMain->powerThread()->CurrentDraw();
			mLink->WriteString( ss.str() );
			break;
		}

		case SET_ROLL_ABSOLUTE : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			value = std::min( std::max( value, -20.0f ), 20.0f );
			mRPY.x = value;
			mLink->WriteString( "" );
			break;
		}
		case SET_PITCH_ABSOLUTE : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			value = std::min( std::max( value, -20.0f ), 20.0f );
			mRPY.y = value;
			mLink->WriteString( "" );
			break;
		}

		case SET_YAW_RELATIVE : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mRPY.z += value;
			mLink->WriteString( "" );
			break;
		}

		case SET_THRUST_RELATIVE : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mThrust += value;
			mLink->WriteString( "", 1 );
			break;
		}
		case RESET_MOTORS : {
			mThrust = 0.0f;
			mMain->frame()->ResetMotors();
			mRPY = Vector3f();
			mLink->WriteString( "", 1 );
			break;
		}

		case SET_PID_P : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->frame()->setP( value );
			mLink->WriteString( "", 1 );
			break;
		}
		case SET_PID_I : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->frame()->setI( value );
			mLink->WriteString( "", 1 );
			break;
		}
		case SET_PID_D : {
			float value;
			if ( mLink->ReadFloat( &value ) < 0 ) {
				break;
			}
			mMain->frame()->setD( value );
			mLink->WriteString( "", 1 );
			break;
		}

		default:
			break;
	}

	usleep( 1000 * 10 );
// 	mTicks = Board::WaitTick( 1000 * 1000 / 30, mTicks );

	return true;
}
