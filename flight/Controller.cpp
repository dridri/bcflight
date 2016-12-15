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
#include "video/Camera.h"

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
	, mTelemetryThread( new HookThread< Controller >( "telemetry", this, &Controller::TelemetryRun ) )
	, mTelemetryTick( 0 )
	, mTelemetryCounter( 0 )
	, mEmergencyTick( 0 )
	, mTelemetryFull( false )
{
	mTelemetryFrequency = main->config()->integer( "controller.telemetry_rate", 20 );

	mExpo = Vector4f();
	mExpo.x = main->config()->number( "controller.expo.roll" );
	mExpo.y = main->config()->number( "controller.expo.pitch" );
	mExpo.z = main->config()->number( "controller.expo.yaw" );
	mExpo.w = main->config()->number( "controller.expo.thrust" );
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

	gDebug() << "Starting TX thread\n";
	Start();
	gDebug() << "Starting RX thread\n";
	mTelemetryThread->Start();
	gDebug() << "Waiting link to be ready\n";
	while ( !mLink->isConnected() ) {
		usleep( 1000 * 100 );
	}
	gDebug() << "Controller ready !\n";
}


Controller::~Controller()
{
}


bool Controller::connected() const
{
	return mConnected;
// 	return mLink->isConnected();
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


void Controller::SendDebug( const std::string& s )
{
	Packet packet( DEBUG_OUTPUT );
	packet.WriteString( s );

	mSendMutex.lock();
	mLink->Write( &packet );
	mSendMutex.unlock();
}


bool Controller::run()
{
/*
	if ( mEmergencyTick != 0 and ( Board::GetTicks() - mEmergencyTick ) > 1 * 1000 * 1000 ) {
		mThrust = 0.0f;
		mMain->stabilizer()->Reset( 0.0f );
		mMain->frame()->Disarm();
		mRPY = Vector3f();
		mSmoothRPY = Vector3f();
		mArmed = false;
		gDebug() << "STONE MODE !\n";
	}
*/
	if ( !mLink->isConnected() ) {
		mConnected = false;
		mRPY.x = 0.0f;
		mRPY.y = 0.0f;
	}

	if ( !mLink->isConnected() ) {
		gDebug() << "Link not up, connecting...\n";
// 		mConnected = false;
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Controller link initialized !\n";
			mEmergencyTick = 0;
		} else {
			usleep( 1000 * 250 );
		}
		return true;
	}

	int readret = 0;
	Packet command;
// 	if ( mLink->Read( &command, 500 ) <= 0 ) {
	if ( ( readret = mLink->Read( &command, 0 ) ) == LINK_ERROR_TIMEOUT ) {
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
		/*
		mMain->stabilizer()->setMode( Stabilizer::Stabilize );
		mMain->imu()->ResetRPY();
		mRPY.x = 0.0f;
		mRPY.y = 0.0f;
		mRPY.z = mMain->imu()->RPY().z;
		if ( mThrust > 0.3f ) {
			mThrust = 0.3f;
		}
		mEmergencyTick = Board::GetTicks();
		gDebug() << "EMERGENCY MODE !\n";
		return true;
		*/
	}

	Cmd cmd = (Cmd)0;
	while ( command.ReadU32( (uint32_t*)&cmd ) > 0 ) {
// 		printf( "Received cmd %08X\n", cmd );
		bool do_response = false;
		Packet response( cmd );

		switch ( cmd )
		{
			case PING : {
				if ( not mConnected ) {
					gDebug() << "Controller connected !\n";
					mConnected = true;
				}
				uint32_t ticks = 0;
				if ( command.ReadU32( &ticks ) == sizeof(uint32_t) ) {
					response.WriteU32( ticks );
					do_response = true;
				}
				break;
			}
			case GET_BOARD_INFOS : {
				std::string res = mMain->board()->infos();
				response.WriteString( res );
				do_response = true;
				break;
			}
			case GET_SENSORS_INFOS : {
				std::string res = Sensor::infosAll();
				response.WriteString( res );
				do_response = true;
				break;
			}
			case GET_CONFIG_FILE : {
				std::string conf = mMain->config()->ReadFile();
				response.WriteU32( crc32( (uint8_t*)conf.c_str(), conf.length() ) );
				response.WriteString( conf );
				do_response = true;
				break;
			}
			case SET_CONFIG_FILE : {
				uint32_t crc = command.ReadU32();
				std::string conf = command.ReadString();
				if ( crc32( (uint8_t*)conf.c_str(), conf.length() ) == crc ) {
					gDebug() << "Received new configuration : " << conf << "\n";
					response.WriteU32( 0 );
					mSendMutex.lock();
					mLink->Write( &response );
					mSendMutex.unlock();
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
						command.Read( (uint32_t*)buf, size / 4 );
						uint32_t local_crc = crc32( buf, size );
						if ( local_crc == crc ) {
							response.WriteU32( 1 );
							mSendMutex.lock();
							mLink->Write( &response );
							mSendMutex.unlock();
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
						while ( mMain->imu()->state() == IMU::Calibrating or mMain->imu()->state() == IMU::CalibratingAll ) {
							usleep( 1000 * 10 );
						}
						response.WriteU32( 0 );
						mMain->frame()->Disarm(); // Activate motors
					}
					do_response = true;
				}
				break;
			}
			case CALIBRATE_ESCS : {
				// TODO : Abort ESC calibration if already done once, or if drone has been armed before
				if ( not mArmed ) {
					mMain->stabilizer()->CalibrateESCs();
				}
				break;
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
				gDebug() << "Arming\n";
				if ( mMain->imu()->state() != IMU::Running ) {
					response.WriteU32( 0 );
					do_response = true;
					break;
				}
				mMain->imu()->ResetYaw();
				mMain->stabilizer()->Reset( mMain->imu()->RPY().z );
				mMain->frame()->Arm();
				mRPY.x = 0.0f;
				mRPY.y = 0.0f;
				mRPY.z = 0.0f;
				mArmed = true;
				response.WriteU32( mArmed );
				do_response = true;
				break;
			}
			case DISARM : {
				gDebug() << "Disarming\n";
				mArmed = false;
				mThrust = 0.0f;
				mMain->stabilizer()->Reset( 0.0f );
				mRPY = Vector3f();
				mSmoothRPY = Vector3f();
				mMain->frame()->Disarm();
				response.WriteU32( mArmed );
				do_response = true;
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
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
// 				if ( std::abs( value ) < 0.05f ) {
// 					value = 0.0f;
// 				}
				if ( value >= 0.0f ) {
					value = ( std::exp( value * mExpo.x ) - 1.0f ) / ( std::exp( mExpo.x ) - 1.0f );
				} else {
					value = -( std::exp( -value * mExpo.x ) - 1.0f ) / ( std::exp( mExpo.x ) - 1.0f );
				}
				mRPY.x = value;
				break;
			}
			case SET_PITCH : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
// 				if ( std::abs( value ) < 0.05f ) {
// 					value = 0.0f;
// 				}
				value = -value; // TEST
				if ( value >= 0.0f ) {
					value = ( std::exp( value * mExpo.y ) - 1.0f ) / ( std::exp( mExpo.y ) - 1.0f );
				} else {
					value = -( std::exp( -value * mExpo.y ) - 1.0f ) / ( std::exp( mExpo.y ) - 1.0f );
				}
				mRPY.y = value;
				break;
			}

			case SET_YAW : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				if ( std::abs( value ) < 0.05f ) {
					value = 0.0f;
				}
				if ( value >= 0.0f ) {
					value = ( std::exp( value * mExpo.z ) - 1.0f ) / ( std::exp( mExpo.z ) - 1.0f );
				} else {
					value = -( std::exp( -value * mExpo.z ) - 1.0f ) / ( std::exp( mExpo.z ) - 1.0f );
				}
/*
				if ( mMain->stabilizer()->mode() == Stabilizer::Stabilize ) {
					mRPY.z += value;
				} else {
					mRPY.z = value;
				}
*/
				mRPY.z = value; // TEST : direct control, no heading
				break;
			}

			case SET_THRUST : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				if ( std::abs( value ) < 0.05f ) {
					value = 0.0f;
				}
				if ( not mMain->stabilizer()->altitudeHold() ) {
					value = std::log( value * ( mExpo.z - 1.0f ) + 1.0f ) / std::log( mExpo.z );
				}
				mThrust = value;
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
					} else if ( mode == (uint32_t)Stabilizer::Stabilize ) {
						mMain->imu()->setRateOnly( false );
						mMain->imu()->ResetRPY();
						mRPY.z = mMain->imu()->RPY().z;
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
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setRollP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_ROLL_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setRollI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_ROLL_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
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
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setPitchP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_PITCH_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setPitchI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_PITCH_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
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
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setYawP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_YAW_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setYawI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_YAW_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
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
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setOuterP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_OUTER_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setOuterI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_OUTER_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
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
				if ( command.ReadFloat( &v.x ) < 0 or command.ReadFloat( &v.y ) < 0 ) {
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
			case GET_RECORDINGS_LIST : {
				response.WriteString( mMain->getRecordingsList() );
				do_response = true;
				break;
			}
			case RECORD_DOWNLOAD : {
				std::string file = command.ReadString();
// 				mMain->UploadFile( filename ); // TODO
				break;
			}

			default: {
				printf( "Controller::run() WARNING : Unknown command 0x%08X\n", cmd );
				break;
			}
		}

		if ( do_response ) {
			mSendMutex.lock();
			mLink->Write( &response );
			mSendMutex.unlock();
		}
	}

	return true;
}


bool Controller::TelemetryRun()
{
	if ( !mLink->isConnected() ) {
		usleep( 1000 * 10 );
		return true;
	}

	Packet telemetry;

	if ( mTelemetryCounter % 3 == 0 ) {
		telemetry.WriteU32( VBAT );
		telemetry.WriteFloat( mMain->powerThread()->VBat() );

		telemetry.WriteU32( TOTAL_CURRENT );
		telemetry.WriteFloat( mMain->powerThread()->CurrentTotal() );

		telemetry.WriteU32( CURRENT_DRAW );
		telemetry.WriteFloat( mMain->powerThread()->CurrentDraw() );

		telemetry.WriteU32( BATTERY_LEVEL );
		telemetry.WriteFloat( mMain->powerThread()->BatteryLevel() );

		telemetry.WriteU32( CPU_LOAD );
		telemetry.WriteU32( Board::CPULoad() );

		telemetry.WriteU32( CPU_TEMP );
		telemetry.WriteU32( Board::CPUTemp() );

		telemetry.WriteU32( RX_QUALITY );
		telemetry.WriteU32( mLink->RxQuality() );
	}

	telemetry.WriteU32( ROLL_PITCH_YAW );
	if ( mMain->stabilizer()->mode() == Stabilizer::Rate ) {
		telemetry.WriteFloat( mMain->imu()->gyroscope().x );
		telemetry.WriteFloat( mMain->imu()->gyroscope().y );
		telemetry.WriteFloat( mMain->imu()->gyroscope().z );
	} else {
		telemetry.WriteFloat( mMain->imu()->RPY().x );
		telemetry.WriteFloat( mMain->imu()->RPY().y );
		telemetry.WriteFloat( mMain->imu()->RPY().z );
	}

	telemetry.WriteU32( CURRENT_ACCELERATION );
	telemetry.WriteFloat( mMain->imu()->acceleration().xyz().length() );

	telemetry.WriteU32( ALTITUDE );
	telemetry.WriteFloat( mMain->imu()->altitude() );

	if ( mTelemetryFull ) {
		telemetry.WriteU32( GYRO );
		telemetry.WriteFloat( mMain->imu()->gyroscope().x );
		telemetry.WriteFloat( mMain->imu()->gyroscope().y );
		telemetry.WriteFloat( mMain->imu()->gyroscope().z );

		telemetry.WriteU32( ACCEL );
		telemetry.WriteFloat( mMain->imu()->acceleration().x );
		telemetry.WriteFloat( mMain->imu()->acceleration().y );
		telemetry.WriteFloat( mMain->imu()->acceleration().z );

		telemetry.WriteU32( MAGN );
		telemetry.WriteFloat( mMain->imu()->magnetometer().x );
		telemetry.WriteFloat( mMain->imu()->magnetometer().y );
		telemetry.WriteFloat( mMain->imu()->magnetometer().z );
	}

	mSendMutex.lock();
	mLink->Write( &telemetry );
	mSendMutex.unlock();
	mTelemetryTick = Board::WaitTick( 1000000 / mTelemetryFrequency, mTelemetryTick );
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
