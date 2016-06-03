#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <iostream>
#include "Controller.h"
#include "links/RawWifi.h"


std::map< Controller::Cmd, std::string > Controller::mCommandsNames = {
	{ Controller::UNKNOWN, "Unknown" },
	// Configure
	{ Controller::PING, "Ping" },
	{ Controller::CALIBRATE, "Calibrate" },
	{ Controller::SET_TIMESTAMP, "Set timestamp" },
	{ Controller::ARM, "Arm" },
	{ Controller::DISARM, "Disarm" },
	{ Controller::RESET_BATTERY, "0x75" },
	{ Controller::CALIBRATE_ESCS, "0x76" },
	{ Controller::SET_FULL_TELEMETRY, "0x77" },
	{ Controller::GET_BOARD_INFOS, "0x80" },
	{ Controller::GET_SENSORS_INFOS, "0x81" },
	// Getters
	{ Controller::PRESSURE, "0x10" },
	{ Controller::TEMPERATURE, "0x11" },
	{ Controller::ALTITUDE, "Altitude" },
	{ Controller::ROLL, "Roll" },
	{ Controller::PITCH, "Pitch" },
	{ Controller::YAW, "Yaw" },
	{ Controller::ROLL_PITCH_YAW, "Roll-Pitch-Yaw" },
	{ Controller::ACCEL, "Acceleration" },
	{ Controller::GYRO, "Gyroscope" },
	{ Controller::MAGN, "Magnetometer" },
	{ Controller::MOTORS_SPEED, "0x1A" },
	{ Controller::CURRENT_ACCELERATION, "Current acceleration" },
	{ Controller::SENSORS_DATA, "0x20" },
	{ Controller::PID_OUTPUT, "0x21" },
	{ Controller::OUTER_PID_OUTPUT, "0x22" },
	{ Controller::PID_FACTORS, "0x23" },
	{ Controller::OUTER_PID_FACTORS, "0x24" },
	{ Controller::HORIZON_OFFSET, "0x25" },
	{ Controller::VBAT, "Battery voltage" },
	{ Controller::TOTAL_CURRENT, "Total current draw" },
	{ Controller::CURRENT_DRAW, "Current draw" },
	{ Controller::BATTERY_LEVEL, "Battery level" },
	{ Controller::CPU_LOAD, "0x35" },
	{ Controller::CPU_TEMP, "0x36" },
	// Setters
	{ Controller::SET_ROLL, "0x40" },
	{ Controller::SET_PITCH, "0x41" },
	{ Controller::SET_YAW, "0x42" },
	{ Controller::SET_THRUST, "0x43" },
	{ Controller::RESET_MOTORS, "0x47" },
	{ Controller::SET_MODE, "0x48" },
	{ Controller::SET_ALTITUDE_HOLD, "Altitude hold" },
	{ Controller::SET_PID_P, "0x50" },
	{ Controller::SET_PID_I, "0x51" },
	{ Controller::SET_PID_D, "0x52" },
	{ Controller::SET_OUTER_PID_P, "0x53" },
	{ Controller::SET_OUTER_PID_I, "0x54" },
	{ Controller::SET_OUTER_PID_D, "0x55" },
	{ Controller::SET_HORIZON_OFFSET, "0x56" },
	// Video
	{ Controller::VIDEO_START_RECORD, "0xA0" },
	{ Controller::VIDEO_STOP_RECORD, "0xA1" },
	{ Controller::VIDEO_GET_BRIGHTNESS, "0xA2" },
	{ Controller::VIDEO_SET_BRIGHTNESS, "0xAA" },
};


static uint32_t htonf( float f )
{
	union {
		float f;
		uint32_t u;
	} u;
	u.f = f;
	return u.u;
}

Controller::Controller( Link* link )
	: Thread( "controller-tx" )
	, mPing( 0 )
	, mTotalCurrent( 0 )
	, mCurrentDraw( 0 )
	, mBatteryVoltage( 0 )
	, mBatteryLevel( 0 )
	, mAltitude( 0.0f )
	, mThrust( 0.0f )
	, mControlRPY{ 0.0f, 0.0f, 0.0f }
	, mLink( link )
	, mConnected( false )
	, mLockState( 0 )
	, mPingTimer( 0 )
	, mMsCounter( 0 )
	, mMsCounter50( 0 )
	, mBoardInfos( "" )
	, mSensorsInfos( "" )
	, mTicks( 0 )
	, mAcceleration( 0.0f )
{
// 	mADC = new MCP320x();
	mArmed = false;
	mMode = Stabilize;
	memset( mSwitches, 0, sizeof( mSwitches ) );

	signal( SIGPIPE, SIG_IGN );
/*
	mJoysticks[0] = Joystick( mADC, 0, 0, true );
	mJoysticks[1] = Joystick( mADC, 1, 1 );
	mJoysticks[2] = Joystick( mADC, 2, 2 );
	mJoysticks[3] = Joystick( mADC, 3, 3 );
*/
	mRxThread = new HookThread<Controller>( "controller-rx", this, &Controller::RxRun );

	Start();
}


Controller::~Controller()
{
	Stop();
	mRxThread->Stop();
}

/*
Controller::Joystick::Joystick( MCP320x* adc, int id, int adc_channel, bool thrust_mode )
	: mADC( adc )
	, mId( id )
	, mADCChannel( adc_channel )
	, mCalibrated( false )
	, mThrustMode( thrust_mode )
	, mMin( 0 )
	, mCenter( 32767 )
	, mMax( 65535 )
{
}


Controller::Joystick::~Joystick()
{
}


void Controller::Joystick::SetCalibratedValues( uint16_t min, uint16_t center, uint16_t max )
{
	mMin = min;
	mCenter = center;
	mMax = max;
	mCalibrated = true;
}


uint16_t Controller::Joystick::ReadRaw()
{
	return mADC->Read( mADCChannel );
}


float Controller::Joystick::Read()
{
	uint16_t raw = ReadRaw();
	if ( raw <= 0 ) {
		return -10.0f;
	}

	if ( mThrustMode ) {
		float ret = (float)( raw - mMin ) / (float)( mMax - mMin );
		ret = std::max( 0.0f, std::min( 1.0f, ret ) );
		return ret;
	}

// 	if ( mADCChannel == 3 ) {
// 		printf( "raw : %04X\n", raw );
// 	}

	float ret = (float)( raw - mCenter ) / (float)( mMax - mCenter );
	ret = std::max( -1.0f, std::min( 1.0f, ret ) );
	return ret;
}
*/

bool Controller::run()
{
	uint64_t ticks0 = Thread::GetTick();


	if ( /*not mConnected or*/ not mLink->isConnected() ) {
		std::cout << "Connecting...";
		mConnected = ( mLink->Connect() == 0 );
		if ( mConnected ) {
			setPriority( 99 );
			std::cout << "Ok !\n";
// 			uint32_t uid = htonl( 0x12345678 );
// 			mLink->Write( &uid, sizeof( uid ) );
			mXferMutex.lock();
			mTxFrame.WriteU32( OUTER_PID_FACTORS );
			mTxFrame.WriteU32( HORIZON_OFFSET );
			mXferMutex.unlock();
		} else {
			std::cout << "Nope !\n";
		}
		return true;
	}


	if ( mLockState >= 1 ) {
		mLockState = 2;
		usleep( 1000 * 10 );
		return true;
	}

	if ( Thread::GetTick() - mPingTimer >= 250 ) {
		uint64_t ticks = Thread::GetTick();
		mXferMutex.lock();
		mTxFrame.WriteU32( PING );
		mTxFrame.WriteU32( (uint32_t)( ticks & 0xFFFFFFFFL ) );
		mXferMutex.unlock();

		if ( not mRxThread->running() ) {
			mRxThread->Start();
		}

// 		uint16_t battery_voltage = mADC->Read( 7 ); // TODO
// 		if ( battery_voltage != 0 ) {
// 			mLocalBatteryVoltage = (float)battery_voltage * 5.0f * 2.0f / 4096.0f;
// 		}

		mPingTimer = Thread::GetTick();
	}

	bool switch_0 = ReadSwitch( 0 );
	bool switch_1 = ReadSwitch( 1 );
	bool switch_2 = ReadSwitch( 2 );
	bool switch_3 = ReadSwitch( 3 );
	bool switch_4 = ReadSwitch( 4 );
	if ( switch_0 and not mSwitches[0] ) {
		std::cout << "Switch 0 on\n";
	} else if ( not switch_0 and mSwitches[0] ) {
		std::cout << "Switch 0 off\n";
	}
	if ( switch_1 and not mSwitches[1] ) {
		std::cout << "Switch 1 on\n";
	} else if ( not switch_1 and mSwitches[1] ) {
		std::cout << "Switch 1 off\n";
	}
	if ( switch_2 and not mSwitches[2] ) {
		Arm();
	} else if ( not switch_2 and mSwitches[2] ) {
		Disarm();
	}
	if ( switch_3 and not mSwitches[3] ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( VIDEO_START_RECORD );
		mXferMutex.unlock();
	} else if ( not switch_3 and mSwitches[3] ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( VIDEO_STOP_RECORD );
		mXferMutex.unlock();
	}
	if ( switch_4 and not mSwitches[4] ) {
		setMode( Rate );
	} else if ( not switch_4 and mSwitches[4] ) {
		setMode( Stabilize );
	}
	mSwitches[0] = switch_0;
	mSwitches[1] = switch_1;
	mSwitches[2] = switch_2;
	mSwitches[3] = switch_3;
	mSwitches[4] = switch_4;

	float r_thrust = ReadThrust();
	float r_yaw = ReadYaw();
	float r_pitch = ReadPitch();
	float r_roll = ReadRoll();
	if ( fabsf( r_thrust - mThrust ) >= 0.01f and r_thrust >= 0.0f and r_thrust <= 1.0f ) {
		setThrust( r_thrust );
	}
	if ( r_yaw >= -1.0f and r_yaw <= 1.0f ) {
		if ( fabsf( r_yaw ) <= 0.05f ) {
			r_yaw = 0.0f;
		}
		if ( fabsf( r_yaw - mControlRPY.z ) >= 0.01f ) {
			setYaw( r_yaw );
		}
	}
	if ( r_pitch >= -1.0f and r_pitch <= 1.0f ) {
		if ( fabsf( r_pitch ) <= 0.05f ) {
			r_pitch = 0.0f;
		}
		if ( fabsf( r_pitch - mControlRPY.y ) >= 0.01f ) {
			setPitch( r_pitch );
		}
	}
	if ( r_roll >= -1.0f and r_roll <= 1.0f ) {
		if ( fabsf( r_roll ) <= 0.05f ) {
			r_roll = 0.0f;
		}
		if ( fabsf( r_roll - mControlRPY.x ) >= 0.01f ) {
			setRoll( r_roll );
		}
	}




	mXferMutex.lock();
	if ( mTxFrame.data().size() > 0 ) {
		mLink->Write( &mTxFrame );
	}
	mTxFrame = Packet();
	mXferMutex.unlock();

	if ( Thread::GetTick() - mTicks < 1000 / 100 ) {
		usleep( 1000 * std::max( 0, 1000 / 100 - (int)( Thread::GetTick() - mTicks ) - 1 ) );
	}
	mTicks = Thread::GetTick();
	mMsCounter += ( mTicks - ticks0 );
	return true;
}


bool Controller::RxRun()
{
	Packet telemetry;
	mLink->Read( &telemetry );
	Cmd cmd = (Cmd)0;

	while ( telemetry.ReadU32( (uint32_t*)&cmd ) > 0 ) {
// 		std::cout << "Received command : " << mCommandsNames[(cmd)] << "\n";

		switch( cmd ) {
			case UNKNOWN : {
				break;
			}
			case PING : {
				if ( mPing == 0 ) {
					setPriority( 99 );
				}
				uint32_t ret = telemetry.ReadU32();
				uint32_t curr = (uint32_t)( Thread::GetTick() & 0xFFFFFFFFL );
				mPing = curr - ret;
				break;
			}
			case CALIBRATE : {
				if ( telemetry.ReadU32() == 0 ) {
					std::cout << "Calibration success\n";
				} else {
					std::cout << "WARNING : Calibration failed !\n";
				}
				break;
			}
			case ARM : {
				mArmed = telemetry.ReadU32();
				break;
			}
			case DISARM : {
				mArmed = telemetry.ReadU32();
				break;
			}
			case RESET_BATTERY : {
				uint32_t unused = telemetry.ReadU32();
				break;
			}

			case GET_BOARD_INFOS : {
				mBoardInfos = telemetry.ReadString();
				break;
			}
			case GET_SENSORS_INFOS : {
				mSensorsInfos = telemetry.ReadString();
				break;
			}

			case VBAT : {
				mBatteryVoltage = telemetry.ReadFloat();
				break;
			}
			case TOTAL_CURRENT : {
				mTotalCurrent = (uint32_t)( telemetry.ReadFloat() * 1000.0f );
				break;
			}
			case CURRENT_DRAW : {
				mCurrentDraw = telemetry.ReadFloat();
				break;
			}
			case BATTERY_LEVEL : {
				mBatteryLevel = telemetry.ReadFloat();
				break;
			}
			case CPU_LOAD : {
				mCPULoad = telemetry.ReadU32();
				break;
			}
			case CPU_TEMP : {
				mCPUTemp = telemetry.ReadU32();
				break;
			}

			case OUTER_PID_FACTORS : {
				mOuterPID.x = telemetry.ReadFloat();
				mOuterPID.y = telemetry.ReadFloat();
				mOuterPID.z = telemetry.ReadFloat();
				break;
			}
			case HORIZON_OFFSET : {
				mHorizonOffset.x = telemetry.ReadFloat();
				mHorizonOffset.y = telemetry.ReadFloat();
				break;
			}
			case SET_OUTER_PID_P : {
				mOuterPID.x = telemetry.ReadFloat();
				break;
			}
			case SET_OUTER_PID_I : {
				mOuterPID.y = telemetry.ReadFloat();
				break;
			}
			case SET_OUTER_PID_D : {
				mOuterPID.z = telemetry.ReadFloat();
				break;
			}
			case SET_HORIZON_OFFSET : {
				mHorizonOffset.x = telemetry.ReadFloat();
				mHorizonOffset.y = telemetry.ReadFloat();
				break;
			}

			case ROLL_PITCH_YAW : {
				mRPY.x = telemetry.ReadFloat();
				mRPY.y = telemetry.ReadFloat();
				mRPY.z = telemetry.ReadFloat();
				mRPYHistory.emplace_back( mRPY );
				if ( mRPYHistory.size() > 256 ) {
					mRPYHistory.pop_front();
				}
				break;
			}
			case CURRENT_ACCELERATION : {
				mAcceleration = telemetry.ReadFloat();
				break;
			}
			case ALTITUDE : {
				mAltitude = telemetry.ReadFloat();
				mAltitudeHistory.emplace_back( mAltitude );
				if ( mAltitudeHistory.size() > 256 ) {
					mAltitudeHistory.pop_front();
				}
				break;
			}
			case SET_MODE : {
				mMode = (Mode)telemetry.ReadU32();
				break;
			}

			default :
				std::cout << "WARNING : Received unknown packet (" << (uint32_t)cmd << ") !\n";
				break;
		}
	}

	return true;
}


void Controller::Calibrate()
{
	if ( !mLink ) {
		return;
	}

	struct {
		uint32_t cmd = 0;
		uint32_t full_recalibration = 0;
		float current_altitude = 0.0f;
	} calibrate_cmd;
	calibrate_cmd.cmd = htonl( CALIBRATE );
	mXferMutex.lock();
	mTxFrame.Write( (uint8_t*)&calibrate_cmd, sizeof( calibrate_cmd ) );
	mXferMutex.unlock();
}


void Controller::CalibrateAll()
{
	if ( !mLink ) {
		return;
	}

	struct {
		uint32_t cmd = 0;
		uint32_t full_recalibration = 1;
		float current_altitude = 0.0f;
	} calibrate_cmd;
	calibrate_cmd.cmd = htonl( CALIBRATE );
	mXferMutex.lock();
	mTxFrame.Write( (uint8_t*)&calibrate_cmd, sizeof( calibrate_cmd ) );
	mXferMutex.unlock();
}


void Controller::CalibrateESCs()
{
	if ( !mLink ) {
		return;
	}

	mXferMutex.lock();
	mTxFrame.WriteU32( CALIBRATE_ESCS );
	mXferMutex.unlock();
}


void Controller::setFullTelemetry( bool fullt )
{
	mXferMutex.lock();
	for ( uint32_t retries = 0; retries < 8; retries++ ) {
		mTxFrame.WriteU32( SET_FULL_TELEMETRY );
		mTxFrame.WriteU32( fullt );
	}
	mXferMutex.unlock();
}


void Controller::Arm()
{
	mXferMutex.lock();
	for ( uint32_t retries = 0; retries < 8; retries++ ) {
		mTxFrame.WriteU32( ARM );
	}
	mXferMutex.unlock();
}


void Controller::Disarm()
{
	mXferMutex.lock();
	for ( uint32_t retries = 0; retries < 8; retries++ ) {
		mTxFrame.WriteU32( DISARM );
	}
	mThrust = 0.0f;
	mXferMutex.unlock();
}


void Controller::ResetBattery()
{
	mXferMutex.lock();
	for ( uint32_t retries = 0; retries < 8; retries++ ) {
		mTxFrame.WriteU32( RESET_BATTERY );
		mTxFrame.WriteU32( 0U );
	}
	mXferMutex.unlock();
}


std::string Controller::getBoardInfos()
{
	// Wait for data to be filled by RX Thread (RxRun())
	while ( mBoardInfos.length() == 0 ) {
		mXferMutex.lock();
		for ( uint32_t retries = 0; retries < 1; retries++ ) {
			mTxFrame.WriteU32( GET_BOARD_INFOS );
		}
		mXferMutex.unlock();
		usleep( 1000 * 250 );
	}

	return mBoardInfos;
}


std::string Controller::getSensorsInfos()
{
	// Wait for data to be filled by RX Thread (RxRun())
	while ( mSensorsInfos.length() == 0 ) {
		mXferMutex.lock();
		for ( uint32_t retries = 0; retries < 1; retries++ ) {
			mTxFrame.WriteU32( GET_SENSORS_INFOS );
		}
		mXferMutex.unlock();
		usleep( 1000 * 250 );
	}

	return mSensorsInfos;
}


void Controller::setPID( const vec3& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_PID_P );
	mTxFrame.WriteFloat( v.x );
	mTxFrame.WriteU32( SET_PID_I );
	mTxFrame.WriteFloat( v.y );
	mTxFrame.WriteU32( SET_PID_D );
	mTxFrame.WriteFloat( v.z );
	mXferMutex.unlock();
}


void Controller::setOuterPID( const vec3& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_OUTER_PID_P );
	mTxFrame.WriteFloat( v.x );
	mTxFrame.WriteU32( SET_OUTER_PID_I );
	mTxFrame.WriteFloat( v.y );
	mTxFrame.WriteU32( SET_OUTER_PID_D );
	mTxFrame.WriteFloat( v.z );
	mXferMutex.unlock();
}


void Controller::setHorizonOffset( const vec3& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_HORIZON_OFFSET );
	mTxFrame.WriteFloat( v.x );
	mTxFrame.WriteFloat( v.y );
	mXferMutex.unlock();
}


void Controller::setThrust( const float& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_THRUST );
	mTxFrame.WriteFloat( v );
	mXferMutex.unlock();
	mThrust = v;
}


void Controller::setThrustRelative( const float& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_THRUST );
	mTxFrame.WriteFloat( mThrust + v );
	mThrust += v;
	mXferMutex.unlock();
}


void Controller::setRoll( const float& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_ROLL );
	mTxFrame.WriteFloat( v );
	mControlRPY.x = v;
	mXferMutex.unlock();
}


void Controller::setPitch( const float& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_PITCH );
	mTxFrame.WriteFloat( v );
	mControlRPY.y = v;
	mXferMutex.unlock();
}



void Controller::setYaw( const float& v )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_YAW );
	mTxFrame.WriteFloat( v );
	mControlRPY.z = v;
	mXferMutex.unlock();
}


void Controller::setMode( const Controller::Mode& mode )
{
	mXferMutex.lock();
	mTxFrame.WriteU32( SET_MODE );
	mTxFrame.WriteU32( (uint32_t)mode );
	mXferMutex.unlock();
}


float Controller::acceleration() const
{
	return mAcceleration;
}


const std::list< vec3 >& Controller::rpyHistory() const
{
	return mRPYHistory;
}


const std::list< vec3 >& Controller::outerPidHistory() const
{
	return mOuterPIDHistory;
}


const std::list< float >& Controller::altitudeHistory() const
{
	return mAltitudeHistory;
}


float Controller::localBatteryVoltage() const
{
	return mLocalBatteryVoltage;
}
