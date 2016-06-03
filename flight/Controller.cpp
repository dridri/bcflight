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

	Start();
	mTelemetryThread->Start();
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
	if ( mEmergencyTick != 0 and ( Board::GetTicks() - mEmergencyTick ) > 1 * 1000 * 1000 ) {
		mThrust = 0.0f;
		mMain->stabilizer()->Reset( 0.0f );
		mMain->frame()->Disarm();
		mRPY = Vector3f();
		mSmoothRPY = Vector3f();
		mArmed = false;
		gDebug() << "STONE MODE !\n";
	}

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
			mEmergencyTick = 0;
		}
		return true;
	}

	Packet command;
// 	if ( mLink->Read( &command, 500 ) <= 0 ) {
	if ( mLink->Read( &command, 0 ) < 0 ) {
		gDebug() << "Controller connection lost !\n";
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
	}

	Cmd cmd = (Cmd)0;
	while ( command.ReadU32( (uint32_t*)&cmd ) > 0 ) {
// 		printf( "Received cmd %08X\n", cmd );
		bool do_response = false;
		Packet response( cmd );

		switch ( cmd )
		{
			case PING : {
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
						response.WriteU32( 0 );
					}
					do_response = true;
				}
				break;
			}
			case CALIBRATE_ESCS : {
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
				mMain->frame()->Disarm();
				mThrust = 0.0f;
				mMain->stabilizer()->Reset( 0.0f );
				mRPY = Vector3f();
				mSmoothRPY = Vector3f();
				mArmed = false;
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
					mMain->stabilizer()->setMode( mode );
					mRPY.x = 0.0f;
					mRPY.y = 0.0f;
					if ( mode == (uint32_t)Stabilizer::Rate ) {
						mRPY.z = 0.0f;
					} else if ( mode == (uint32_t)Stabilizer::Stabilize ) {
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

			case SET_PID_P : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setP( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_PID_I : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setI( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case SET_PID_D : {
				float value;
				if ( command.ReadFloat( &value ) < 0 ) {
					break;
				}
				mMain->stabilizer()->setD( value );
				response.WriteFloat( value );
				do_response = true;
				break;
			}
			case PID_FACTORS : {
				Vector3f pid = mMain->stabilizer()->getPID();
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
				}
				break;
			}
			case VIDEO_STOP_RECORD : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					cam->StopRecording();
				}
				break;
			}
			case VIDEO_GET_BRIGHTNESS : {
				Camera* cam = mMain->camera();
				if ( cam ) {
					response.WriteU32( cam->brightness() );
					do_response = true;
				}
				break;
			}
			case VIDEO_SET_BRIGHTNESS : {
				uint32_t value;
				if ( command.ReadU32( &value ) < 0 ) {
					break;
				}
				Camera* cam = mMain->camera();
				if ( cam ) {
					cam->setBrightness( value );
				}
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
	}

	telemetry.WriteU32( ROLL_PITCH_YAW );
	telemetry.WriteFloat( mMain->imu()->RPY().x );
	telemetry.WriteFloat( mMain->imu()->RPY().y );
	telemetry.WriteFloat( mMain->imu()->RPY().z );

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
	mTelemetryTick = Board::WaitTick( 1000000 / 10, mTelemetryTick );
	mTelemetryCounter++;
	return true;
}
