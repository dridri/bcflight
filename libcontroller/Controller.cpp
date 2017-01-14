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
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <math.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>
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
	{ Controller::DEBUG_OUTPUT, "0x7A" },
	{ Controller::GET_BOARD_INFOS, "0x80" },
	{ Controller::GET_SENSORS_INFOS, "0x81" },
	{ Controller::GET_CONFIG_FILE, "0x90" },
	{ Controller::SET_CONFIG_FILE, "0x91" },
	{ Controller::UPDATE_UPLOAD_INIT, "0x9A" },
	{ Controller::UPDATE_UPLOAD_DATA, "0x9B" },
	{ Controller::UPDATE_UPLOAD_PROCESS, "0x9C" },
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
	{ Controller::ROLL_PID_FACTORS, "0x23" },
	{ Controller::PITCH_PID_FACTORS, "0x24" },
	{ Controller::YAW_PID_FACTORS, "0x25" },
	{ Controller::OUTER_PID_FACTORS, "0x26" },
	{ Controller::HORIZON_OFFSET, "0x27" },
	{ Controller::VBAT, "Battery voltage" },
	{ Controller::TOTAL_CURRENT, "Total current draw" },
	{ Controller::CURRENT_DRAW, "Current draw" },
	{ Controller::BATTERY_LEVEL, "Battery level" },
	{ Controller::CPU_LOAD, "0x35" },
	{ Controller::CPU_TEMP, "0x36" },
	{ Controller::RX_QUALITY, "0x37" },
	// Setters
	{ Controller::SET_ROLL, "0x40" },
	{ Controller::SET_PITCH, "0x41" },
	{ Controller::SET_YAW, "0x42" },
	{ Controller::SET_THRUST, "0x43" },
	{ Controller::RESET_MOTORS, "0x47" },
	{ Controller::SET_MODE, "0x48" },
	{ Controller::SET_ALTITUDE_HOLD, "Altitude hold" },
	{ Controller::SET_ROLL_PID_P, "0x50" },
	{ Controller::SET_ROLL_PID_I, "0x51" },
	{ Controller::SET_ROLL_PID_D, "0x52" },
	{ Controller::SET_PITCH_PID_P, "0x53" },
	{ Controller::SET_PITCH_PID_I, "0x54" },
	{ Controller::SET_PITCH_PID_D, "0x55" },
	{ Controller::SET_YAW_PID_P, "0x56" },
	{ Controller::SET_YAW_PID_I, "0x57" },
	{ Controller::SET_YAW_PID_D, "0x58" },
	{ Controller::SET_OUTER_PID_P, "0x59" },
	{ Controller::SET_OUTER_PID_I, "0x5A" },
	{ Controller::SET_OUTER_PID_D, "0x5B" },
	{ Controller::SET_HORIZON_OFFSET, "0x5C" },
	// Video
	{ Controller::VIDEO_START_RECORD, "0xA0" },
	{ Controller::VIDEO_STOP_RECORD, "0xA1" },
	{ Controller::VIDEO_BRIGHTNESS_INCR, "0xA4" },
	{ Controller::VIDEO_BRIGHTNESS_DECR, "0xA5" },
	{ Controller::VIDEO_CONTRAST_INCR, "0xA6" },
	{ Controller::VIDEO_CONTRAST_DECR, "0xA7" },
	{ Controller::VIDEO_SATURATION_INCR, "0xA8" },
	{ Controller::VIDEO_SATURATION_DECR, "0xA9" },
};

Controller::Controller( Link* link, bool spectate )
	: Thread( "controller-tx" )
	, mPing( 0 )
	, mCalibrated( false )
	, mArmed( false )
	, mTotalCurrent( 0 )
	, mCurrentDraw( 0 )
	, mBatteryVoltage( 0 )
	, mBatteryLevel( 0 )
	, mCPULoad( 0 )
	, mCPUTemp( 0 )
	, mAltitude( 0.0f )
	, mThrust( 0.0f )
	, mControlRPY{ 0.0f, 0.0f, 0.0f }
	, mPIDsLoaded( false )
	, mLink( link )
	, mSpectate( spectate )
	, mConnected( false )
	, mConnectionEstablished( false )
	, mLockState( 0 )
	, mTickBase( Thread::GetTick() )
	, mPingTimer( 0 )
	, mDataTimer( 0 )
	, mMsCounter( 0 )
	, mMsCounter50( 0 )
	, mBoardInfos( "" )
	, mSensorsInfos( "" )
	, mConfigFile( "" )
	, mRecordingsList( "" )
	, mUpdateUploadValid( false )
	, mConfigUploadValid( false )
	, mTicks( 0 )
	, mSwitches{ 0 }
	, mVideoRecording( false )
	, mAcceleration( 0.0f )
{
	mMode = Rate;
	memset( mSwitches, 0, sizeof( mSwitches ) );

#ifndef WIN32
	signal( SIGPIPE, SIG_IGN );
#endif
	mRxThread = new HookThread<Controller>( "controller-rx", this, &Controller::RxRun );
	mRxThread->setPriority( 98, 1 );

	if ( spectate ) {
		Stop();
		mRxThread->Start();
	} else {
		Start();
	}
}


Controller::~Controller()
{
	Stop();
	mRxThread->Stop();
}


std::string Controller::debugOutput()
{
	std::string ret = "";

	mDebugMutex.lock();
	ret = mDebug;
	mDebug = "";
	mDebugMutex.unlock();

	return ret;
}


bool Controller::run()
{
	uint64_t ticks0 = Thread::GetTick();

	if ( not mLink ) {
		usleep( 1000 * 500 );
		return true;
	}

	if ( not mLink->isConnected() ) {
		std::cout << "Connecting...";
		mConnected = ( mLink->Connect() == 0 );
		if ( mConnected ) {
			setPriority( 99, 1 );
			std::cout << "Ok !\n" << std::flush;
// 			uint32_t uid = htonl( 0x12345678 );
// 			mLink->Write( &uid, sizeof( uid ) );
		} else {
			std::cout << "Nope !\n" << std::flush;
			usleep( 1000 * 250 );
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
		mTxFrame.WriteU32( mPing ); // Report current ping
		mLink->Write( &mTxFrame );
		mXferMutex.unlock();

		if ( not mRxThread->running() ) {
			mRxThread->Start();
			mXferMutex.lock();
			mTxFrame.WriteU32( ROLL_PID_FACTORS );
			mTxFrame.WriteU32( PITCH_PID_FACTORS );
			mTxFrame.WriteU32( YAW_PID_FACTORS );
			mTxFrame.WriteU32( OUTER_PID_FACTORS );
			mTxFrame.WriteU32( HORIZON_OFFSET );
			mXferMutex.unlock();
		}

		mPingTimer = Thread::GetTick();
	}

	// Send controls at 10Hz, allowing to be sure that TRPY are well received by the drone
	bool force_update_controls = false;
	if ( Thread::GetTick() - mDataTimer >= 1000 / 10 ) {
		force_update_controls = true;
		mDataTimer = Thread::GetTick();
	}

	uint32_t oldswitch[8];
	memcpy( oldswitch, mSwitches, sizeof(oldswitch) );
	for ( uint32_t i = 0; i < 8; i++ ) {
		bool on = ReadSwitch( i );
		if ( on and not mSwitches[i] ) {
			std::cout << "Switch " << i << " on\n" << std::flush;
		} else if ( not on and mSwitches[i] ) {
			std::cout << "Switch " << i << " off\n" << std::flush;
		}
		mSwitches[i] = on;
	}

	if ( mSwitches[2] and not mArmed and mCalibrated ) {
		Arm();
	} else if ( not mSwitches[2] and mArmed ) {
		Disarm();
	}
	if ( mSwitches[3] and mMode != Stabilize ) {
		setMode( Stabilize );
	} else if ( not mSwitches[3] and mMode != Rate ) {
		setMode( Rate );
	}
	if ( mSwitches[5] and not mVideoRecording ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( VIDEO_START_RECORD );
		mXferMutex.unlock();
	} else if ( not mSwitches[5] and mVideoRecording ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( VIDEO_STOP_RECORD );
		mXferMutex.unlock();
	}

	float r_thrust = ReadThrust();
	float r_yaw = ReadYaw();
	float r_pitch = ReadPitch();
	float r_roll = ReadRoll();
	if ( ( force_update_controls or fabsf( r_thrust - mThrust ) >= 0.01f ) and r_thrust >= 0.0f and r_thrust <= 1.0f ) {
		setThrust( r_thrust );
	}
	if ( r_yaw >= -1.0f and r_yaw <= 1.0f ) {
		if ( fabsf( r_yaw ) <= 0.05f ) {
			r_yaw = 0.0f;
		}
		if ( ( force_update_controls or fabsf( r_yaw - mControlRPY.z ) >= 0.01f ) ) {
			setYaw( r_yaw );
		}
	}
	if ( r_pitch >= -1.0f and r_pitch <= 1.0f ) {
		if ( fabsf( r_pitch ) <= 0.05f ) {
			r_pitch = 0.0f;
		}
		else if ( r_pitch < 0.0f ) {
			r_pitch += 0.05f;
		} else if ( r_pitch > 0.0f ) {
			r_pitch -= 0.05f;
		}
		if ( ( force_update_controls or fabsf( r_pitch - mControlRPY.y ) >= 0.01f ) ) {
			setPitch( r_pitch );
		}
	}
	if ( r_roll >= -1.0f and r_roll <= 1.0f ) {
		if ( fabsf( r_roll ) <= 0.05f ) {
			r_roll = 0.0f;
		}
		else if ( r_roll < 0.0f ) {
			r_roll += 0.05f;
		} else if ( r_roll > 0.0f ) {
			r_roll -= 0.05f;
		}
		if ( ( force_update_controls or fabsf( r_roll - mControlRPY.x ) >= 0.01f ) ) {
			setRoll( r_roll );
		}
	}

	mXferMutex.lock();
	if ( mTxFrame.data().size() > 0 ) {
		mLink->Write( &mTxFrame );
	}
	mTxFrame = Packet();
	mXferMutex.unlock();

	if ( Thread::GetTick() - mTicks < 1000 / 200 ) {
		usleep( 1000 * std::max( 0, 1000 / 200 - (int)( Thread::GetTick() - mTicks ) - 1 ) );
	}
	mTicks = Thread::GetTick();
	mMsCounter += ( mTicks - ticks0 );
	return true;
}


bool Controller::RxRun()
{
	if ( mSpectate and not mLink->isConnected() ) {
		std::cout << "Connecting..." << std::flush;
		mConnected = ( mLink->Connect() == 0 );
		if ( mConnected ) {
			std::cout << "Ok !\n" << std::flush;
		} else {
			std::cout << "Nope !\n" << std::flush;
			usleep( 1000 * 250 );
		}
		return true;
	}

	Packet telemetry;
	if ( mLink->Read( &telemetry ) <= 0 ) {
		usleep( 500 );
		return true;
	}
	Cmd cmd = (Cmd)0;

	while ( telemetry.ReadU32( (uint32_t*)&cmd ) > 0 ) {
// 		std::cout << "Received command : " << mCommandsNames[(cmd)] << "\n";

		switch( cmd ) {
			case UNKNOWN : {
				break;
			}
			case PING : {
				uint32_t ret = telemetry.ReadU32();
				uint32_t reported_ping = telemetry.ReadU32();
				uint32_t curr = (uint32_t)( Thread::GetTick() & 0xFFFFFFFFL );
				mPing = curr - ret;
				if ( mSpectate ) {
					mPing = reported_ping;
				}
				mConnectionEstablished = true;
				break;
			}
			case DEBUG_OUTPUT : {
				mDebugMutex.lock();
				std::string str = telemetry.ReadString();
				std::cout << str << std::flush;
				mDebug += str;
				mDebugMutex.unlock();
				break;
			}
			case CALIBRATE : {
				if ( telemetry.ReadU32() == 0 ) {
					mCalibrated = true;
					std::cout << "Calibration success\n" << std::flush;
				} else {
					std::cout << "WARNING : Calibration failed !\n" << std::flush;
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
				(void)unused;
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
			case GET_CONFIG_FILE : {
				uint32_t crc = telemetry.ReadU32();
				std::string content = telemetry.ReadString();
				if ( crc32( (uint8_t*)content.c_str(), content.length() ) == crc ) {
					mConfigFile = content;
				} else {
					std::cout << "Received broken config flie, retrying...\n" << std::flush;
					mConfigFile = "";
				}
				break;
			}
			case SET_CONFIG_FILE : {
				mConfigUploadValid = ( telemetry.ReadU32() == 0 );
				break;
			}
			case UPDATE_UPLOAD_DATA : {
				bool ok = ( telemetry.ReadU32() == 1 );
				std::cout << "UPDATE_UPLOAD_DATA : " << ok << "\n" << std::flush;
				mUpdateUploadValid = ok;
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
			case RX_QUALITY : {
				mDroneRxQuality = telemetry.ReadU32();
				break;
			}

			case ROLL_PID_FACTORS : {
				mRollPID.x = telemetry.ReadFloat();
				mRollPID.y = telemetry.ReadFloat();
				mRollPID.z = telemetry.ReadFloat();
				mPIDsLoaded = true;
				break;
			}
			case PITCH_PID_FACTORS : {
				mPitchPID.x = telemetry.ReadFloat();
				mPitchPID.y = telemetry.ReadFloat();
				mPitchPID.z = telemetry.ReadFloat();
				mPIDsLoaded = true;
				break;
			}
			case YAW_PID_FACTORS : {
				mYawPID.x = telemetry.ReadFloat();
				mYawPID.y = telemetry.ReadFloat();
				mYawPID.z = telemetry.ReadFloat();
				mPIDsLoaded = true;
				break;
			}
			case OUTER_PID_FACTORS : {
				mOuterPID.x = telemetry.ReadFloat();
				mOuterPID.y = telemetry.ReadFloat();
				mOuterPID.z = telemetry.ReadFloat();
				mPIDsLoaded = true;
				break;
			}
			case HORIZON_OFFSET : {
				mHorizonOffset.x = telemetry.ReadFloat();
				mHorizonOffset.y = telemetry.ReadFloat();
				break;
			}
			case SET_ROLL_PID_P : {
				mRollPID.x = telemetry.ReadFloat();
				break;
			}
			case SET_ROLL_PID_I : {
				mRollPID.y = telemetry.ReadFloat();
				break;
			}
			case SET_ROLL_PID_D : {
				mRollPID.z = telemetry.ReadFloat();
				break;
			}
			case SET_PITCH_PID_P : {
				mPitchPID.x = telemetry.ReadFloat();
				break;
			}
			case SET_PITCH_PID_I : {
				mPitchPID.y = telemetry.ReadFloat();
				break;
			}
			case SET_PITCH_PID_D : {
				mPitchPID.z = telemetry.ReadFloat();
				break;
			}
			case SET_YAW_PID_P : {
				mYawPID.x = telemetry.ReadFloat();
				break;
			}
			case SET_YAW_PID_I : {
				mYawPID.y = telemetry.ReadFloat();
				break;
			}
			case SET_YAW_PID_D : {
				mYawPID.z = telemetry.ReadFloat();
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

			case SET_THRUST : {
				float value = telemetry.ReadFloat();
				if ( mSpectate ) {
					mThrust = value;
				}
				break;
			}

			case ROLL_PITCH_YAW : {
				mRPY.x = telemetry.ReadFloat();
				mRPY.y = telemetry.ReadFloat();
				mRPY.z = telemetry.ReadFloat();
				vec4 rpy;
				rpy.x = mRPY.x;
				rpy.y = mRPY.y;
				rpy.z = mRPY.z;
				rpy.w = (double)( Thread::GetTick() - mTickBase ) / 1000.0;
				mRPYHistory.emplace_back( rpy );
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

			case VIDEO_START_RECORD : {
				mVideoRecording = telemetry.ReadU32();
				break;
			}
			case VIDEO_STOP_RECORD : {
				mVideoRecording = telemetry.ReadU32();
				break;
			}
			case GET_RECORDINGS_LIST : {
				uint32_t crc = telemetry.ReadU32();
				std::string content = telemetry.ReadString();
				if ( crc32( (uint8_t*)content.c_str(), content.length() ) == crc ) {
					mRecordingsList = content;
				} else {
					std::cout << "Received broken recordings list, retrying...\n";
					mRecordingsList = "broken";
				}
				break;
			}

			default :
				std::cout << "WARNING : Received unknown packet (" << (uint32_t)cmd << ") !\n" << std::flush;
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
	for ( uint32_t retries = 0; retries < 1; retries++ ) {
		mTxFrame.WriteU32( ARM );
	}
	mXferMutex.unlock();
}


void Controller::Disarm()
{
	mXferMutex.lock();
	for ( uint32_t retries = 0; retries < 1; retries++ ) {
		mTxFrame.WriteU32( DISARM );
	}
	mThrust = 0.0f;
	mXferMutex.unlock();
}


void Controller::ResetBattery()
{
	mXferMutex.lock();
	for ( uint32_t retries = 0; retries < 1; retries++ ) {
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


std::string Controller::getConfigFile()
{
	mConfigFile = "";

	// Wait for data to be filled by RX Thread (RxRun())
	while ( mConfigFile.length() == 0 ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( GET_CONFIG_FILE );
		mXferMutex.unlock();
		usleep( 1000 * 250 );
	}

	return mConfigFile;
}


void Controller::setConfigFile( const std::string& content )
{
	Packet packet( SET_CONFIG_FILE );
	packet.WriteU32( crc32( (uint8_t*)content.c_str(), content.length() ) );
	packet.WriteString( content );

	mConfigUploadValid = false;
	while ( not mConfigUploadValid ) {
		mXferMutex.lock();
// 		mTxFrame.WriteU32( SET_CONFIG_FILE );
// 		mTxFrame.WriteString( content );
		mLink->Write( &packet );
		mXferMutex.unlock();
		usleep( 1000 * 250 );
	};
	mConfigFile = "";
}


void Controller::UploadUpdateInit()
{
	Packet init( UPDATE_UPLOAD_INIT );
	for ( uint32_t retries = 0; retries < 8; retries++ ) {
		mXferMutex.lock();
		mLink->Write( &init );
		mXferMutex.unlock();
		usleep( 1000 * 50 );
	}
}


void Controller::UploadUpdateData( const uint8_t* buf, uint32_t offset, uint32_t size )
{

	Packet packet( UPDATE_UPLOAD_DATA );
	packet.WriteU32( crc32( buf, size ) );
	packet.WriteU32( offset );
	packet.WriteU32( offset );
	packet.WriteU32( size );
	packet.WriteU32( size );
	packet.Write( buf, size );


	mUpdateUploadValid = false;
	mXferMutex.lock();
	int retries = 1;
#ifndef NO_RAWWIFI
	if ( dynamic_cast< RawWifi* >( mLink ) != nullptr ) {
		retries = dynamic_cast< RawWifi* >( mLink )->retriesCount();
		dynamic_cast< RawWifi* >( mLink )->setRetriesCount( 1 );
	}
#endif
	do {
		mLink->Write( &packet );
		usleep( 1000 * 50 );
		if ( not mUpdateUploadValid ) {
			std::cout << "Broken data received, retrying...\n" << std::flush;
			usleep( 1000 * 10 );
		}
	} while ( not mUpdateUploadValid );

	if ( dynamic_cast< RawWifi* >( mLink ) != nullptr ) {
		dynamic_cast< RawWifi* >( mLink )->setRetriesCount( retries );
	}
	mXferMutex.unlock();

}


void Controller::UploadUpdateProcess( const uint8_t* buf, uint32_t size )
{
	Packet process( UPDATE_UPLOAD_PROCESS );
	process.WriteU32( crc32( buf, size ) );
	for ( uint32_t retries = 0; retries < 8; retries++ ) {
		mXferMutex.lock();
		mLink->Write( &process );
		mXferMutex.unlock();
		usleep( 1000 * 50 );
	}
}


void Controller::EnableTunDevice()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( ENABLE_TUN_DEVICE );
	mXferMutex.unlock();
}


void Controller::DisableTunDevice()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( DISABLE_TUN_DEVICE );
	mXferMutex.unlock();
}


void Controller::ReloadPIDs()
{
	for ( uint32_t retries = 0; retries < 4; retries++ ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( ROLL_PID_FACTORS );
		mTxFrame.WriteU32( PITCH_PID_FACTORS );
		mTxFrame.WriteU32( YAW_PID_FACTORS );
		mTxFrame.WriteU32( OUTER_PID_FACTORS );
		mXferMutex.unlock();
		usleep( 1000 * 10 );
	}
}


void Controller::setRollPID( const vec3& v )
{
	std::cout << "setRollPID...\n" << std::flush;
	while ( mRollPID.x != v.x or mRollPID.y != v.y or mRollPID.z != v.z ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( SET_ROLL_PID_P );
		mTxFrame.WriteFloat( v.x );
		mTxFrame.WriteU32( SET_ROLL_PID_I );
		mTxFrame.WriteFloat( v.y );
		mTxFrame.WriteU32( SET_ROLL_PID_D );
		mTxFrame.WriteFloat( v.z );
		mXferMutex.unlock();
		usleep( 1000 * 100 );
	}
	std::cout << "setRollPID ok\n" << std::flush;
}


void Controller::setPitchPID( const vec3& v )
{
	std::cout << "setPitchPID...\n" << std::flush;
	while ( mPitchPID.x != v.x or mPitchPID.y != v.y or mPitchPID.z != v.z ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( SET_PITCH_PID_P );
		mTxFrame.WriteFloat( v.x );
		mTxFrame.WriteU32( SET_PITCH_PID_I );
		mTxFrame.WriteFloat( v.y );
		mTxFrame.WriteU32( SET_PITCH_PID_D );
		mTxFrame.WriteFloat( v.z );
		mXferMutex.unlock();
		usleep( 1000 * 100 );
	}
	std::cout << "setPitchPID ok\n" << std::flush;
}


void Controller::setYawPID( const vec3& v )
{
	std::cout << "setYawPID...\n" << std::flush;
	while ( mYawPID.x != v.x or mYawPID.y != v.y or mYawPID.z != v.z ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( SET_YAW_PID_P );
		mTxFrame.WriteFloat( v.x );
		mTxFrame.WriteU32( SET_YAW_PID_I );
		mTxFrame.WriteFloat( v.y );
		mTxFrame.WriteU32( SET_YAW_PID_D );
		mTxFrame.WriteFloat( v.z );
		mXferMutex.unlock();
		usleep( 1000 * 100 );
	}
	std::cout << "setYawPID ok\n" << std::flush;
}


void Controller::setOuterPID( const vec3& v )
{
	std::cout << "setOuterPID...\n" << std::flush;
	while ( mOuterPID.x != v.x or mOuterPID.y != v.y or mOuterPID.z != v.z ) {
		mXferMutex.lock();
		mTxFrame.WriteU32( SET_OUTER_PID_P );
		mTxFrame.WriteFloat( v.x );
		mTxFrame.WriteU32( SET_OUTER_PID_I );
		mTxFrame.WriteFloat( v.y );
		mTxFrame.WriteU32( SET_OUTER_PID_D );
		mTxFrame.WriteFloat( v.z );
		mXferMutex.unlock();
		usleep( 1000 * 100 );
	}
	std::cout << "setOuterPID ok\n" << std::flush;
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


void Controller::VideoBrightnessIncrease()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( VIDEO_BRIGHTNESS_INCR );
	mXferMutex.unlock();
}


void Controller::VideoBrightnessDecrease()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( VIDEO_BRIGHTNESS_DECR );
	mXferMutex.unlock();
}


void Controller::VideoContrastIncrease()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( VIDEO_CONTRAST_INCR );
	mXferMutex.unlock();
}


void Controller::VideoContrastDecrease()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( VIDEO_CONTRAST_DECR );
	mXferMutex.unlock();
}


void Controller::VideoSaturationIncrease()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( VIDEO_SATURATION_INCR );
	mXferMutex.unlock();
}


void Controller::VideoSaturationDecrease()
{
	mXferMutex.lock();
	mTxFrame.WriteU32( VIDEO_SATURATION_DECR );
	mXferMutex.unlock();
}


std::vector< std::string > Controller::recordingsList()
{
	mRecordingsList = "";

	// Wait for data to be filled by RX Thread (RxRun())
	do {
		mXferMutex.lock();
		mTxFrame.WriteU32( GET_RECORDINGS_LIST );
		mXferMutex.unlock();
		while ( mRecordingsList.length() == 0 ) {
			usleep( 1000 * 250 );
		}
	} while ( mRecordingsList == "broken" );

	std::vector< std::string > list;
	std::string full = mRecordingsList;
	while ( full.length() > 0 ) {
		list.emplace_back( full.substr( 0, full.find( ";" ) ) );
		full = full.substr( full.find( ";" ) + 1 );
	}
	return list;
}


float Controller::acceleration() const
{
	return mAcceleration;
}


const std::list< vec4 >& Controller::rpyHistory() const
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


uint32_t Controller::crc32( const uint8_t* buf, uint32_t len )
{
	uint32_t k = 0;
	uint32_t crc = 0;

	crc = ~crc;

	while ( len-- ) {
		crc ^= *buf++;
		for ( k = 0; k < 8; k++ ) {
			crc = ( crc & 1 ) ? ( (crc >> 1) ^ 0x82f63b78 ) : ( crc >> 1 );
		}
	}

	return ~crc;
}
