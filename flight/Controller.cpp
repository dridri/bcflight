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
#include <stdio.h>
#include <arpa/inet.h>
#include <algorithm>
#include "Main.h"
#include "Config.h"
#include "Controller.h"
#include <Link.h>
#include <IMU.h>
#include <Gyroscope.h>
#include <Accelerometer.h>
#include <Magnetometer.h>
#include <Sensor.h>
#include <Stabilizer.h>
#include <Frame.h>
#include "video/Camera.h"

Controller::Controller( Main* main, Link* link )
	: ControllerBase( link )
	, Thread( "controller" )
	, mMain( main )
	, mTimedOut( false )
	, mArmed( false )
	, mPing( 0 )
	, mRPY( Vector3f() )
	, mThrust( 0.0f )
	, mTicks( 0 )
	, mTelemetryThread( new HookThread< Controller >( "telemetry", this, &Controller::TelemetryRun ) )
	, mTelemetryTick( 0 )
	, mTelemetryCounter( 0 )
	, mEmergencyTick( 0 )
	, mTelemetryFull( main->config()->Boolean( "controller.full_telemetry", false ) )
{
	mTelemetryFrequency = main->config()->Integer( "controller.telemetry_rate", 20 );

	mExpo = Vector4f();
	mExpo.x = main->config()->Number( "controller.expo.roll" );
	mExpo.y = main->config()->Number( "controller.expo.pitch" );
	mExpo.z = main->config()->Number( "controller.expo.yaw" );
	mExpo.w = main->config()->Number( "controller.expo.thrust" );
	if ( mExpo.x < 0.01f ) {
		mExpo.x = 0.01f;
	}
	if ( mExpo.y < 0.01f ) {
		mExpo.y = 0.01f;
	}
	if ( mExpo.z < 0.01f ) {
		mExpo.z = 0.01f;
	}
	if ( mExpo.w < 1.01f ) {
		mExpo.w = 1.01f;
	}
	if ( mExpo.x <= 0.0f ) {
		mExpo.x = 3.0f;
	}
	if ( mExpo.y <= 0.0f ) {
		mExpo.y = 3.0f;
	}
	if ( mExpo.z <= 0.0f ) {
		mExpo.z = 3.0f;
	}
	if ( mExpo.w <= 0.0f ) {
		mExpo.w = 2.0f;
	}

	gDebug() << "Starting RX thread\n";
	Start();
	if ( mTelemetryFrequency > 0 ) {
		gDebug() << "Starting telemetry thread\n";
		mTelemetryThread->setFrequency( mTelemetryFrequency );
		mTelemetryThread->Start();
	}
// 	gDebug() << "Waiting link to be ready\n";
// 	while ( !mLink->isConnected() ) {
// 		usleep( 1000 * 100 );
// 	}
	gDebug() << "Controller ready !\n";
}


Controller::~Controller()
{
}


Link* Controller::link() const
{
	return mLink;
}


bool Controller::connected() const
{
	return isConnected();
}


bool Controller::armed() const
{
	return mArmed;
}


uint32_t Controller::ping() const
{
	return mPing;
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
	gDebug() << "EMERGENCY MODE !\n";
	mMain->stabilizer()->Reset( 0.0f );
	mMain->frame()->Disarm();
	mThrust = 0.0f;
	mSmoothRPY = Vector3f();
	mRPY = Vector3f();
	mMain->stabilizer()->Reset( 0.0f );
	mMain->frame()->Disarm();
}


void Controller::UpdateSmoothControl( const float& dt )
{
// 	mSmoothControl.Predict( dt );
// 	mSmoothRPY = mSmoothControl.Update( dt, mRPY );
}


void Controller::SendDebug( const string& s )
{
/*
	Packet packet( DEBUG_OUTPUT );
	packet.WriteString( s );

	mSendMutex.lock();
	mLink->Write( &packet );
	mSendMutex.unlock();
*/
}


void Controller::setRoll( float value )
{
	if ( value >= 0.0f ) {
		value = ( exp( value * mExpo.x ) - 1.0f ) / ( exp( mExpo.x ) - 1.0f );
	} else {
		value = -( exp( -value * mExpo.x ) - 1.0f ) / ( exp( mExpo.x ) - 1.0f );
	}
	mRPY.x = value;
}


void Controller::setPitch( float value )
{
	if ( value >= 0.0f ) {
		value = ( exp( value * mExpo.y ) - 1.0f ) / ( exp( mExpo.y ) - 1.0f );
	} else {
		value = -( exp( -value * mExpo.y ) - 1.0f ) / ( exp( mExpo.y ) - 1.0f );
	}
	mRPY.y = value;
}


void Controller::setYaw( float value )
{
	if ( abs( value ) < 0.05f ) {
		value = 0.0f;
	}
	if ( value >= 0.0f ) {
		value = ( exp( value * mExpo.z ) - 1.0f ) / ( exp( mExpo.z ) - 1.0f );
	} else {
		value = -( exp( -value * mExpo.z ) - 1.0f ) / ( exp( mExpo.z ) - 1.0f );
	}
	mRPY.z = value;
}


void Controller::setThrust( float value )
{
	if ( abs( value ) < 0.05f ) {
		value = 0.0f;
	}
	if ( not mMain->stabilizer()->altitudeHold() ) {
		value = log( value * ( mExpo.z - 1.0f ) + 1.0f ) / log( mExpo.z );
		if ( value < 0.0f or isnan( value ) or ( isinf( value ) and value < 0.0f ) ) {
			value = 0.0f;
		}
		if ( value > 1.0f or isinf( value ) ) {
			value = 1.0f;
		}
		mThrust = value;
	}
}


void Controller::Arm() {
	gDebug() << "Arming\n";
	mMain->imu()->ResetYaw();
	mMain->stabilizer()->Reset( mMain->imu()->RPY().z );
	mMain->frame()->Arm();
	mRPY.x = 0.0f;
	mRPY.y = 0.0f;
	mRPY.z = 0.0f;
	mArmed = true;
}


void Controller::Disarm() {
	gDebug() << "Disarming\n";
	mArmed = false;
	mThrust = 0.0f;
	mMain->stabilizer()->Reset( 0.0f );
	mRPY = Vector3f();
	mSmoothRPY = Vector3f();
	mMain->frame()->Disarm();
}


bool Controller::run()
{
	char stmp[64];

	if ( !mLink ) {
		return true;
	}

	if ( !mLink->isConnected() ) {
		mConnected = false;
		mRPY.x = 0.0f;
		mRPY.y = 0.0f;

		gDebug() << "Link not up, connecting...\n";
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Controller link initialized !\n";
			mEmergencyTick = 0;
		} else {
			usleep( 1000 * 250 );
		}
		return true;
	}

	SyncReturn readret = CONTINUE;
	Packet command;
	if ( ( readret = mLink->Read( &command, 0 ) ) == TIMEOUT ) {
		if ( not mTimedOut ) {
			mMain->blackbox()->Enqueue( "Controller:connected", "false" );
		}
		mTimedOut = true;
		if ( mArmed ) {
			gDebug() << "Controller connection lost !\n";
			mThrust = 0.0f;
			mMain->stabilizer()->Reset( 0.0f );
			mMain->frame()->Disarm();
			mRPY = Vector3f();
			mSmoothRPY = Vector3f();
			mArmed = false;
			gDebug() << "STONE MODE !\n";
			return true;
		}
	} else if ( readret == CONTINUE ) {
		return true;
	} else {
		if ( mTimedOut ) {
			if ( mConnected ) {
				mMain->blackbox()->Enqueue( "Controller:connected", "true" );
			}
			mTimedOut = false;
		}
	}

	mMain->blackbox()->Enqueue( "Link:quality", to_string(mLink->RxQuality()) + "% @" + to_string(mLink->RxLevel()) + "dBm" );

    auto ReadCmd = []( Packet* command, Cmd* cmd ) {
		uint8_t part1 = 0;
		uint8_t part2 = 0;
		if ( command->ReadU8( &part1 ) == sizeof(uint8_t) ) {
			if ( ( part1 & SHORT_COMMAND ) == SHORT_COMMAND ) {
				*cmd = (Cmd)part1;
				return (int)sizeof(uint8_t);
			}
			if ( command->ReadU8( &part2 ) == sizeof(uint8_t) ) {
				*cmd = (Cmd)ntohs( ( ((uint16_t)part2) << 8 ) | part1 );
				return (int)sizeof(uint16_t);
			}
		}
		return 0;
	};

	Cmd cmd = (Cmd)0;
	bool acknowledged = false;
	while ( ReadCmd( &command, &cmd ) > 0 ) {
// 		if ( cmd != PING and cmd != TELEMETRY and cmd != CONTROLS and cmd != STATUS ) {
// 			cout << "Received command (" << hex << (int)cmd << dec << ") : " << mCommandsNames[(cmd)] << "\n";
// 		}
		bool do_response = false;
		Packet response;
		if ( ( cmd & SHORT_COMMAND ) == SHORT_COMMAND ) {
			response.WriteU8( cmd );
		} else {
			response.WriteU16( cmd );
		}

		if ( ( cmd & ACK_ID ) == ACK_ID ) {
			uint16_t id = cmd & ~ACK_ID;
			if ( id == mRXAckID ) {
				acknowledged = true;
			} else {
				acknowledged = false;
			}
			mRXAckID = id;
			continue;
		}

		if ( not mConnected ) {
			if ( cmd == PING ) {
				gDebug() << "Controller connected !\n";
				mConnected = true;
				mConnectionEstablished = true;
				mMain->blackbox()->Enqueue( "Controller:connected", "true" );
				mMain->blackbox()->Enqueue( "Controller:armed", mArmed ? "true" : "false" );
			} else {
				gDebug() << "Ignoring command " << hex << (int)cmd << dec << "\n";
				continue;
			}
		}

		switch ( cmd )
		{
			case PING : {
				uint16_t ticks = 0;
				if ( command.ReadU16( &ticks ) == sizeof(uint16_t) ) {
					response.WriteU16( ticks );
					uint16_t last = command.ReadU16();
					mPing = last;
					response.WriteU16( last ); // Copy-back reported ping
					mMain->blackbox()->Enqueue( "Controller:ping", to_string(mPing) + "ms" );

					// Send status
					uint32_t status = 0;
					if ( mArmed ) {
						status |= STATUS_ARMED;
					}
					if ( mMain->imu()->state() == IMU::Running ) {
						status |= STATUS_CALIBRATED;
					} else if ( mMain->imu()->state() == IMU::Calibrating or mMain->imu()->state() == IMU::CalibratingAll ) {
						status |= STATUS_CALIBRATING;
					}

					if ( mMain->camera() ) {
						if ( mMain->camera()->nightMode() ) {
							status |= STATUS_NIGHTMODE;
						}
					}
					response.WriteU8( STATUS );
					response.WriteU32( status );

					// Send telemetry
					Telemetry telemetry;
					telemetry.battery_voltage = (uint16_t)( mMain->powerThread()->VBat() * 100.0f );
					telemetry.total_current = (uint16_t)( mMain->powerThread()->CurrentTotal() * 1000.0f );
					telemetry.current_draw = (uint8_t)( mMain->powerThread()->CurrentDraw() * 10.0f );
					telemetry.battery_level = (uint8_t)( mMain->powerThread()->BatteryLevel() * 100.0f );
					telemetry.cpu_load = Board::CPULoad();
					telemetry.cpu_temp = Board::CPUTemp();
					telemetry.rx_quality = mLink->RxQuality();
					telemetry.rx_level = mLink->RxLevel();
					response.WriteU8( TELEMETRY );
					response.Write( (uint8_t*)&telemetry, sizeof(telemetry) );

					do_response = true;
				}
				break;
			}
			case CONTROLS : {
				if ( not mConnected ) {
					// Should never happen (except at first controller connection)
					break;
				}
				Controls controls = { 0, 0, 0, 0, 0, 0, 0, 0 };
				if ( command.Read( (uint8_t*)&controls, sizeof(controls) ) == sizeof(controls) ) {
					if ( controls.arm and not mArmed and mMain->imu()->state() == IMU::Running ) {
						Arm();
						mMain->blackbox()->Enqueue( "Controller:armed", mArmed ? "true" : "false" );
					} else if ( not controls.arm and mArmed ) {
						Disarm();
						mMain->blackbox()->Enqueue( "Controller:armed", mArmed ? "true" : "false" );
					}
					if ( mArmed ) {
						setThrust( ((float)controls.thrust) / 127.0f );
						setRoll( ((float)controls.roll) / 128.0f );
						setPitch( ((float)controls.pitch) / 128.0f );
						setYaw( ((float)controls.yaw) / 128.0f );
						sprintf( stmp, "\"%.4f,%.4f,%.4f,%.4f\"", mThrust, mRPY.x, mRPY.y, mRPY.z );
						mMain->blackbox()->Enqueue( "Controller:trpy", stmp );
					}
				}
				break;
			}
			case GET_BOARD_INFOS : {
				string res = mMain->board()->infos();
				response.WriteString( res );
				do_response = true;
				break;
			}
			case GET_SENSORS_INFOS : {
				string res = Sensor::infosAll();
				response.WriteString( res );
				do_response = true;
				break;
			}
			case GET_CONFIG_FILE : {
				string conf = mMain->config()->ReadFile();
				response.WriteU32( crc32( (uint8_t*)conf.c_str(), conf.length() ) );
				response.WriteString( conf );
				do_response = true;
				break;
			}
			case SET_CONFIG_FILE : {
				uint32_t crc = command.ReadU32();
				string conf = command.ReadString();
				if ( crc32( (uint8_t*)conf.c_str(), conf.length() ) == crc ) {
					gDebug() << "Received new configuration : " << conf << "\n";
					response.WriteU32( 0 );
#ifdef SYSTEM_NAME_Linux
					mSendMutex.lock();
#endif
					mLink->Write( &response );
#ifdef SYSTEM_NAME_Linux
					mSendMutex.unlock();
#endif
					mMain->config()->WriteFile( conf );
					mMain->board()->Reset();
				} else {
					gDebug() << "Received broken configuration\n";
					response.WriteU32( 1 );
					do_response = true;
				}
				break;
			}
			case UPDATE_UPLOAD_INIT : {
				gDebug() << "UPDATE_UPLOAD_INIT\n";
				if ( mTelemetryThread->running() ) {
					mTelemetryThread->Pause();
					gDebug() << "Telemetry thread paused\n";
				}
				break;
			}
			case UPDATE_UPLOAD_DATA : {
				gDebug() << "UPDATE_UPLOAD_DATA\n";
				uint32_t crc = command.ReadU32();
				uint32_t offset = command.ReadU32();
				uint32_t offset2 = command.ReadU32();
				uint32_t size = command.ReadU32();
				uint32_t size2 = command.ReadU32();
				if ( offset == offset2 and offset < 4 * 1024 * 1024 ) {
					if ( size == size2 and size < 4 * 1024 * 1024 ) {
						uint8_t* buf = new uint8_t[ size + 32 ];
						command.Read( (uint8_t*)buf, size );
						uint32_t local_crc = crc32( buf, size );
						if ( local_crc == crc ) {
							response.WriteU32( 1 );
#ifdef SYSTEM_NAME_Linux
							mSendMutex.lock();
#endif
							mLink->Write( &response );
#ifdef SYSTEM_NAME_Linux
							mSendMutex.unlock();
#endif
							Board::UpdateFirmwareData( buf, offset, size );
						} else {
							gDebug() << "Firmware upload CRC32 is invalid (corrupted WiFi frame ?)\n";
							response.WriteU32( 2 );
							do_response = true;
						}
					} else {
						gDebug() << "Firmware upload size is incoherent (corrupted WiFi frame ?)\n";
						response.WriteU32( 3 );
						do_response = true;
					}
				} else {
					gDebug() << "Firmware upload offset is incoherent (corrupted WiFi frame ?)\n";
					response.WriteU32( 4 );
					do_response = true;
				}
				break;
			}
			case UPDATE_UPLOAD_PROCESS : {
				gDebug() << "UPDATE_UPLOAD_PROCESS\n";
				uint32_t crc = command.ReadU32();
				gDebug() << "Processing firmware update...\n";
				Board::UpdateFirmwareProcess( crc );
				break;
			}
			case ENABLE_TUN_DEVICE : {
				gDebug() << "Enabling tun device...\n";
				Board::EnableTunDevice();
				break;
			}
			case DISABLE_TUN_DEVICE : {
				gDebug() << "Disabling tun device...\n";
				Board::DisableTunDevice();
				break;
			}
			case CALIBRATE : {
				uint32_t full_recalibration = 0;
				float curr_altitude = 0.0f;
				if ( command.ReadU32( &full_recalibration ) == sizeof(uint32_t) and command.ReadFloat( &curr_altitude ) == sizeof(float) ) {
					if ( mArmed ) {
						response.WriteU32( 0xFFFFFFFF );
					} else {
						if ( full_recalibration ) {
							mMain->imu()->RecalibrateAll();
						} else {
							mMain->imu()->Recalibrate();
						}
						response.WriteU32( 3 );
					}
					do_response = true;
				}
				response.WriteU16( CALIBRATING );
				response.WriteU32( 1 );
				break;
			}
			case CALIBRATE_ESCS : {
				// TODO : Abort ESC calibration if already done once, or if drone has been armed before
				if ( not mArmed ) {
					mMain->stabilizer()->CalibrateESCs();
				}
				break;
			}
			case MOTOR_TEST : {
				// test motors
				uint32_t id = command.ReadU32();
				if ( not mArmed ) {
					mMain->stabilizer()->MotorTest(id);
				}
			}
			case SET_TIMESTAMP : {
				uint32_t timestamp = 0;
				if ( command.ReadU32( &timestamp ) == sizeof(timestamp) ) {
					mMain->board()->setLocalTimestamp( timestamp );
					response.WriteU32( 0 );
					do_response = true;
				}
				break;
			}
			case SET_FULL_TELEMETRY : {
				uint32_t full = 0;
				if ( command.ReadU32( &full ) == sizeof(full) ) {
					mTelemetryFull = full;
				}
				break;
			}
			case ARM : {
				/*
				if ( mMain->imu()->state() != IMU::Running ) {
					response.WriteU32( 0 );
					do_response = true;
					break;
				}
				Arm();
				mMain->blackbox()->Enqueue( "Controller:armed", mArmed ? "true" : "false" );
				response.WriteU32( mArmed );
				do_response = true;
				*/
				break;
			}
			case DISARM : {
				/*
				if ( mMain->imu()->state() == IMU::Off ) {
					response.WriteU32( 0 );
					do_response = true;
					break;
				}
				Disarm();
				mMain->blackbox()->Enqueue( "Controller:armed", mArmed ? "true" : "false" );
				response.WriteU32( mArmed );
				do_response = true;
				*/
				break;
			}
			case RESET_BATTERY : {
				uint32_t value = 0;
				if ( command.ReadU32( &value ) == sizeof(value) ) {
					mMain->powerThread()->ResetFullBattery( value );
				}
				break;
			}
			case SET_ROLL : {
				float value;
				if ( command.ReadFloat( &value ) == 0 ) {
					break;
				}
				setRoll( value );
				sprintf( stmp, "%.4f", mRPY.x );
				mMain->blackbox()->Enqueue( "Controller:roll", stmp );
				break;
			}
			case SET_PITCH : {
				float value;
				if ( command.ReadFloat( &value ) == 0 ) {
					break;
				}
				setPitch( value );
				sprintf( stmp, "%.4f", mRPY.y );
				mMain->blackbox()->Enqueue( "Controller:pitch", stmp );
				break;
			}

			case SET_YAW : {
				float value;
				if ( command.ReadFloat( &value ) == 0 ) {
					break;
				}
				setYaw( value );
				sprintf( stmp, "%.4f", mRPY.z );
				mMain->blackbox()->Enqueue( "Controller:yaw", stmp );
				break;
			}

			case SET_THRUST : {
				float value;
				if ( command.ReadFloat( &value ) == 0 ) {
					break;
				}
				setThrust( value );
				sprintf( stmp, "%.4f", mThrust );
				mMain->blackbox()->Enqueue( "Controller:thrust", stmp );
				break;
			}

			case RESET_MOTORS : {
				mThrust = 0.0f;
				mMain->stabilizer()->Reset( 0.0f );
				mMain->frame()->Disarm();
				mRPY = Vector3f();
				break;
			}

			case SET_MODE : {
				uint32_t mode = 0;
				if( command.ReadU32( &mode ) == sizeof(uint32_t) ) {
					gDebug() << "SET_MODE : " << mode << "\n";
					mMain->stabilizer()->setMode( mode );
					mRPY.x = 0.0f;
					mRPY.y = 0.0f;
					if ( mode == (uint32_t)Stabilizer::Rate ) {
						mMain->imu()->setRateOnly( true );
						mRPY.z = 0.0f;
						mMain->blackbox()->Enqueue( "Controller:mode", "Rate" );
					} else if ( mode == (uint32_t)Stabilizer::Stabilize ) {
						mMain->imu()->setRateOnly( false );
						mMain->imu()->ResetRPY();
						mRPY.z = mMain->imu()->RPY().z;
						mMain->blackbox()->Enqueue( "Controller:mode", "Stabilize" );
					}
					response.WriteU32( mMain->stabilizer()->mode() );
					do_response = true;
				}
				break;
			}

			case SET_ALTITUDE_HOLD : {
				uint32_t enabled = 0;
				if( command.ReadU32( &enabled ) == sizeof(uint32_t) ) {
					mMain->stabilizer()->setAltitudeHold( enabled != 0 and Sensor::Altimeters().size() > 0 );
					response.WriteU32( mMain->stabilizer()->altitudeHold() );
					do_response = true;
				}
				break;
			}

			case SET_ROLL_PID_P : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setRollP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_ROLL_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setRollI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_ROLL_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setRollD( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case ROLL_PID_FACTORS : {
				Vector3f pid = mMain->stabilizer()->getRollPID();
				response.WriteFloat( pid.x );
				response.WriteFloat( pid.y );
				response.WriteFloat( pid.z );
				do_response = true;
				break;
			}
			case SET_PITCH_PID_P : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setPitchP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_PITCH_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setPitchI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_PITCH_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setPitchD( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case PITCH_PID_FACTORS : {
				Vector3f pid = mMain->stabilizer()->getPitchPID();
				response.WriteFloat( pid.x );
				response.WriteFloat( pid.y );
				response.WriteFloat( pid.z );
				do_response = true;
				break;
			}
			case SET_YAW_PID_P : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setYawP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_YAW_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setYawI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_YAW_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 1.0f ) {
					break;
				}
				mMain->stabilizer()->setYawD( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case YAW_PID_FACTORS : {
				Vector3f pid = mMain->stabilizer()->getYawPID();
				response.WriteFloat( pid.x );
				response.WriteFloat( pid.y );
				response.WriteFloat( pid.z );
				do_response = true;
				break;
			}
			case PID_OUTPUT : {
				Vector3f pid = mMain->stabilizer()->lastPIDOutput();
				response.WriteFloat( pid.x );
				response.WriteFloat( pid.y );
				response.WriteFloat( pid.z );
				do_response = true;
				break;
			}

			case SET_OUTER_PID_P : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 50.0f ) {
					break;
				}
				mMain->stabilizer()->setOuterP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_OUTER_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 50.0f ) {
					break;
				}
				mMain->stabilizer()->setOuterI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_OUTER_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) == 0 or abs(value) > 50.0f ) {
					break;
				}
				mMain->stabilizer()->setOuterD( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case OUTER_PID_FACTORS : {
				Vector3f pid = mMain->stabilizer()->getOuterPID();
				response.WriteFloat( pid.x );
				response.WriteFloat( pid.y );
				response.WriteFloat( pid.z );
				do_response = true;
				break;
			}
			case OUTER_PID_OUTPUT : {
				Vector3f pid = mMain->stabilizer()->lastOuterPIDOutput();
				response.WriteFloat( pid.x );
				response.WriteFloat( pid.y );
				response.WriteFloat( pid.z );
				do_response = true;
				break;
			}
			case SET_HORIZON_OFFSET : {
				Vector3f v;
				if ( command.ReadFloat( &v.x ) == 0 or command.ReadFloat( &v.y ) == 0 or abs(v.x) > 45.0f or abs(v.y) > 45.0f ) {
					break;
				}
				mMain->stabilizer()->setHorizonOffset( v );
				response.WriteFloat( v.x );
				response.WriteFloat( v.y );
				do_response = true;
				break;
			}
			case HORIZON_OFFSET : {
				response.WriteFloat( mMain->stabilizer()->horizonOffset().x );
				response.WriteFloat( mMain->stabilizer()->horizonOffset().y );
				do_response = true;
				break;
			}

			case VIDEO_PAUSE : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					cam->Pause();
					gDebug() << "Video paused\n";
				}
				response.WriteU32( 1 );
				do_response = true;
				break;
			}
			case VIDEO_RESUME : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					cam->Resume();
					gDebug() << "Video resumed\n";
				}
				response.WriteU32( 0 );
				do_response = true;
				break;
			}
			case VIDEO_START_RECORD : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					cam->StartRecording();
					gDebug() << "Video recording started\n";
				}
				response.WriteU32( 1 );
				do_response = true;
				break;
			}
			case VIDEO_STOP_RECORD : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					cam->StopRecording();
					gDebug() << "Video recording stopped\n";
				}
				response.WriteU32( 0 );
				do_response = true;
				break;
			}
			case VIDEO_TAKE_PICTURE : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					cam->TakePicture();
					gDebug() << "Picture taken\n";
				}
				break;
			}
			case VIDEO_BRIGHTNESS_INCR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera brightness to " << cam->brightness() + 1 << "\n";
					cam->setBrightness( cam->brightness() + 1 );
				}
				break;
			}
			case VIDEO_BRIGHTNESS_DECR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera brightness to " << cam->brightness() - 1 << "\n";
					cam->setBrightness( cam->brightness() - 1 );
				}
				break;
			}
			case VIDEO_CONTRAST_INCR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera contrast to " << cam->contrast() + 1 << "\n";
					cam->setContrast( cam->contrast() + 1 );
				}
				break;
			}
			case VIDEO_CONTRAST_DECR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera contrast to " << cam->contrast() - 1 << "\n";
					cam->setContrast( cam->contrast() - 1 );
				}
				break;
			}
			case VIDEO_SATURATION_INCR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera saturation to " << cam->saturation() + 1 << "\n";
					cam->setSaturation( cam->saturation() + 1 );
				}
				break;
			}
			case VIDEO_SATURATION_DECR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera saturation to " << cam->saturation() - 1 << "\n";
					cam->setSaturation( cam->saturation() - 1 );
				}
				break;
			}
			case VIDEO_ISO_INCR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera ISO to " << cam->ISO() + 100 << "\n";
					cam->setISO( cam->ISO() + 100 );
				}
				break;
			}
			case VIDEO_ISO_DECR : {
				Camera* cam = mMain->camera();
				if ( cam and cam->ISO() > 0 ) {
					gDebug() << "Setting camera ISO to " << cam->ISO() - 100 << "\n";
					cam->setISO( cam->ISO() - 100 );
				}
				break;
			}
			case VIDEO_SHUTTER_SPEED_INCR : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					gDebug() << "Setting camera shutter speed to " << cam->shutterSpeed() + 100 << "\n";
					cam->setShutterSpeed( cam->shutterSpeed() + 100 );
				}
				break;
			}
			case VIDEO_SHUTTER_SPEED_DECR : {
				Camera* cam = mMain->camera();
				if ( cam and cam->shutterSpeed() > 0 ) {
					gDebug() << "Setting camera shutter speed to " << cam->shutterSpeed() - 100 << "\n";
					cam->setShutterSpeed( cam->shutterSpeed() - 100 );
				}
				break;
			}
			case VIDEO_ISO : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					response.WriteU32( (uint32_t)cam->ISO() );
					do_response = true;
				}
				break;
			}
			case VIDEO_SHUTTER_SPEED : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					response.WriteU32( cam->shutterSpeed() );
					do_response = true;
				}
				break;
			}
			case VIDEO_LENS_SHADER : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					Camera::LensShaderColor r, g, b;
					cam->getLensShader( &r, &g, &b );
					response.WriteU8( r.base );
					response.WriteU8( r.radius );
					response.WriteU8( r.strength );
					response.WriteU8( g.base );
					response.WriteU8( g.radius );
					response.WriteU8( g.strength );
					response.WriteU8( b.base );
					response.WriteU8( b.radius );
					response.WriteU8( b.strength );
					do_response = true;
				}
				break;
			}
			case VIDEO_SET_LENS_SHADER : {
				Camera::LensShaderColor r, g, b;
				r.base = command.ReadU8();
				r.radius = command.ReadU8();
				r.strength = command.ReadU8();
				g.base = command.ReadU8();
				g.radius = command.ReadU8();
				g.strength = command.ReadU8();
				b.base = command.ReadU8();
				b.radius = command.ReadU8();
				b.strength = command.ReadU8();
				auto int35_float = []( int32_t value ) {
					if ( value < 0 ) {
						return -1.0f * ( (float)( (-value) >> 5 ) + (float)( (-value) & 0b00011111 ) / 32.0f );
					}
					return (float)( value >> 5 ) + (float)( value & 0b00011111 ) / 32.0f;
				};
				if ( r.base != 0 and g.base != 0 and b.base != 0 and r.strength != 0 and g.strength != 0 and b.strength != 0 ) {
					mMain->config()->setNumber( "camera.lens_shading.r.base", int35_float( r.base ) );
					mMain->config()->setInteger( "camera.lens_shading.r.radius", r.radius );
					mMain->config()->setNumber( "camera.lens_shading.r.strength", int35_float( r.strength ) );
					mMain->config()->setNumber( "camera.lens_shading.g.base", int35_float( g.base ) );
					mMain->config()->setInteger( "camera.lens_shading.g.radius", g.radius );
					mMain->config()->setNumber( "camera.lens_shading.g.strength", int35_float( g.strength ) );
					mMain->config()->setNumber( "camera.lens_shading.b.base", int35_float( b.base ) );
					mMain->config()->setInteger( "camera.lens_shading.b.radius", b.radius );
					mMain->config()->setNumber( "camera.lens_shading.b.strength", int35_float( b.strength ) );
					mMain->config()->Save();
					Camera* cam = mMain->camera();
					if ( cam ) {
						cam->setLensShader( r, g, b );
					}
				}
				break;
			}
			case VIDEO_NIGHT_MODE : {
				Camera* cam = mMain->camera();
				uint32_t night = command.ReadU32();
				if ( cam ) {
					gDebug() << "Setting camera to " << ( night ? "night" : "day" ) << " mode\n";
					cam->setNightMode( night );
				}
				response.WriteU32( night );
				do_response = true;
				break;
			}
			case VIDEO_EXPOSURE_MODE : {
				Camera* cam = mMain->camera();
				string ret = "(none)";
				if ( cam ) {
					if ( acknowledged ) {
						ret = cam->exposureMode();
					} else {
						ret = cam->switchExposureMode();
						gDebug() << "Camera exposure mode set to \"" << ret << "\"\n";
					}
				}
				response.WriteString( ret );
				do_response = true;
				break;
			}
			case VIDEO_WHITE_BALANCE : {
				Camera* cam = mMain->camera();
				string ret = "(none)";
				if ( cam ) {
					if ( acknowledged ) {
						ret = cam->whiteBalance();
					} else {
						ret = cam->switchWhiteBalance();
						gDebug() << "Camera white balance set to \"" << ret << "\"\n";
					}
				}
				response.WriteString( ret );
				do_response = true;
				break;
			}
			case VIDEO_LOCK_WHITE_BALANCE : {
				Camera* cam = mMain->camera();
				string ret = "(none)";
				if ( cam ) {
					if ( acknowledged ) {
						ret = cam->whiteBalance();
					} else {
						ret = cam->lockWhiteBalance();
						gDebug() << "Camera white balance \"" << ret << "\"\n";
					}
				}
				response.WriteString( ret );
				do_response = true;
				break;
			}
			case GET_RECORDINGS_LIST : {
				string rec = mMain->getRecordingsList();
				response.WriteU32( crc32( (uint8_t*)rec.c_str(), rec.length() ) );
				response.WriteString( rec );
				do_response = true;
				break;
			}
/*
			case RECORD_DOWNLOAD : {
				string file = command.ReadString();
// 				mMain->UploadFile( filename ); // TODO
				break;
			}
*/
			case GET_USERNAME : {
				if ( mMain->username() != "" ) {
					response.WriteString( mMain->username() );
				} else {
					response.WriteString( "[unknown]" );
				}
				do_response = true;
				break;
			}

			default: {
				printf( "Controller::run() WARNING : Unknown command 0x%08X\n", cmd );
				break;
			}
		}

		if ( do_response ) {
#ifdef SYSTEM_NAME_Linux
			mSendMutex.lock();
#endif
			mLink->Write( &response );
#ifdef SYSTEM_NAME_Linux
			mSendMutex.unlock();
#endif
		}
	}

	return true;
}


bool Controller::TelemetryRun()
{
	if ( !mLink or !mLink->isConnected() ) {
		usleep( 1000 * 10 );
		return true;
	}

	Packet telemetry;

	if ( mTelemetryCounter % 10 == 0 ) {
		telemetry.WriteU16( STABILIZER_FREQUENCY );
		telemetry.WriteU32( mMain->loopFrequency() );

		vector< Motor* >* motors = mMain->frame()->motors();
		if ( motors ) {
			telemetry.WriteU16( MOTORS_SPEED );
			telemetry.WriteU32( motors->size() );
			for ( Motor* m : *motors ) {
				telemetry.WriteFloat( m->speed() );
			}
		}

#ifdef CAMERA
		if ( mMain->cameraType() != "" ) {
			telemetry.WriteU16( ERROR_CAMERA_MISSING );
		}
#endif
	}

	if ( mTelemetryCounter % 5 == 0 ) {
		telemetry.WriteU16( VBAT );
		telemetry.WriteFloat( mMain->powerThread()->VBat() );

		telemetry.WriteU16( TOTAL_CURRENT );
		telemetry.WriteFloat( mMain->powerThread()->CurrentTotal() );

		telemetry.WriteU16( CURRENT_DRAW );
		telemetry.WriteFloat( mMain->powerThread()->CurrentDraw() );

		telemetry.WriteU16( BATTERY_LEVEL );
		telemetry.WriteFloat( mMain->powerThread()->BatteryLevel() );

		telemetry.WriteU16( CPU_LOAD );
		telemetry.WriteU32( Board::CPULoad() );

		telemetry.WriteU16( CPU_TEMP );
		telemetry.WriteU32( Board::CPUTemp() );

		telemetry.WriteU16( RX_QUALITY );
		telemetry.WriteU32( mLink->RxQuality() );

		telemetry.WriteU16( RX_LEVEL );
		telemetry.WriteU32( mLink->RxLevel() );
	}

	telemetry.WriteU16( SET_THRUST );
	telemetry.WriteFloat( mThrust );

	telemetry.WriteU16( ROLL_PITCH_YAW );
		telemetry.WriteFloat( mMain->imu()->RPY().x );
		telemetry.WriteFloat( mMain->imu()->RPY().y );
		telemetry.WriteFloat( mMain->imu()->RPY().z );

	telemetry.WriteU16( CURRENT_ACCELERATION );
	telemetry.WriteFloat( mMain->imu()->acceleration().xyz().length() );

	telemetry.WriteU16( ALTITUDE );
	telemetry.WriteFloat( mMain->imu()->altitude() );

	if ( mTelemetryFull ) {
		telemetry.WriteU16( GYRO );
		telemetry.WriteFloat( mMain->imu()->gyroscope().x );
		telemetry.WriteFloat( mMain->imu()->gyroscope().y );
		telemetry.WriteFloat( mMain->imu()->gyroscope().z );

		telemetry.WriteU16( ACCEL );
		telemetry.WriteFloat( mMain->imu()->acceleration().x );
		telemetry.WriteFloat( mMain->imu()->acceleration().y );
		telemetry.WriteFloat( mMain->imu()->acceleration().z );

		telemetry.WriteU16( MAGN );
		telemetry.WriteFloat( mMain->imu()->magnetometer().x );
		telemetry.WriteFloat( mMain->imu()->magnetometer().y );
		telemetry.WriteFloat( mMain->imu()->magnetometer().z );
	}

#ifdef SYSTEM_NAME_Linux
	mSendMutex.lock();
#endif
	mLink->Write( &telemetry );
#ifdef SYSTEM_NAME_Linux
	mSendMutex.unlock();
#endif
// 	mTelemetryTick = Board::WaitTick( 1000000 / mTelemetryFrequency, mTelemetryTick );
	mTelemetryCounter++;
	return true;
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
