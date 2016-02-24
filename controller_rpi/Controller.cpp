#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <wiringPi.h>
#include <gammaengine/Time.h>
#include <gammaengine/Debug.h>
#include "Controller.h"
#include "ui/Globals.h"


static uint32_t htonf( float f )
{
	union {
		float f;
		uint32_t u;
	} u;
	u.f = f;
	return u.u;
}

Controller::Controller( const std::string& addr, uint16_t port )
	: Thread()
	, mPing( 0 )
	, mTotalCurrent( 0 )
	, mCurrentDraw( 0 )
	, mBatteryVoltage( 0 )
	, mBatteryLevel( 0 )
	, mThrust( 0.0f )
	, mControlRPY( Vector3f( 0.0f, 0.0f, 0.0f ) )
	, mServer( addr )
	, mPort( port )
	, mSocket( nullptr )
	, mConnected( false )
	, mLockState( 0 )
	, mPingTimer( Timer() )
	, mMsCounter( 0 )
	, mMsCounter50( 0 )
	, mTicks( 0 )
	, mAcceleration( 0.0f )
{
	mADC = new MCP320x();
	mArmed = false;
	mMode = Stabilize;
	memset( mSwitches, 0, sizeof( mSwitches ) );

	wiringPiSetup();
	pinMode( 21, INPUT );
	pinMode( 22, INPUT );
	pinMode( 23, INPUT );
	pinMode( 24, INPUT );
	pinMode( 25, INPUT );

	mJoysticks[0] = Joystick( mADC, 0, 0, true );
	mJoysticks[1] = Joystick( mADC, 1, 1 );
	mJoysticks[2] = Joystick( mADC, 2, 2 );
	mJoysticks[3] = Joystick( mADC, 3, 3 );
	mPingTimer.Start();
	Start();
}


Controller::~Controller()
{
}


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
	mMin = getGlobals()->setting( "Joystick:" + std::to_string( mId ) + ":min", 0 );
	mCenter = getGlobals()->setting( "Joystick:" + std::to_string( mId ) + ":cen", 0 );
	mMax = getGlobals()->setting( "Joystick:" + std::to_string( mId ) + ":max", 0 );
	if ( mMin != 0 and mCenter != 0 and mMax != 0 ) {
		mCalibrated = true;
	}
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
	getGlobals()->setSetting( "Joystick:" + std::to_string( mId ) + ":min", mMin );
	getGlobals()->setSetting( "Joystick:" + std::to_string( mId ) + ":cen", mCenter );
	getGlobals()->setSetting( "Joystick:" + std::to_string( mId ) + ":max", mMax );
	getGlobals()->SaveSettings( "/root/ge/settings.txt" );
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

// 	if ( mADCChannel == 1 ) {
// 		printf( "raw : %d\n", raw );
// 	}

	float ret = (float)( raw - mCenter ) / (float)( mMax - mCenter );
	ret = std::max( -1.0f, std::min( 1.0f, ret ) );
	return ret;
}


bool Controller::run()
{
	uint64_t ticks0 = Time::GetTick();

	if ( !mConnected or !mSocket ) {
		gDebug() << "Waiting for IP address\n";
		system( "killall -9 dhclient && dhclient wlan0 && while ! ifconfig | grep -F \"192.168.32.\" > /dev/null; do sleep 1; done" );
		gDebug() << "Connecting...";
		if ( mSocket ) {
			delete mSocket;
		}
		mSocket = new Socket();
		mConnected = ( mSocket->Connect( mServer, mPort, Socket::TCP ) == 0 );
		if ( mConnected ) {
			Debug() << "Ok !\n";
			int flag = 1; 
			setsockopt( mSocket->rawSocket(), IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int) );
		} else {
			Debug() << "Nope !\n";
		}
		return true;
	}

	if ( mLockState == 1 ) {
		mLockState = 2;
	}
	if ( mLockState == 2 ) {
		usleep( 1000 );
		return true;
	}

	if ( mMsCounter - mMsCounter50 >= 50 ) {
		Send( ROLL_PITCH_YAW );
		mRPY.x = ReceiveFloat();
		mRPY.y = ReceiveFloat();
		mRPY.z = ReceiveFloat();
		Send( CURRENT_ACCELERATION );
		mAcceleration = ReceiveFloat();
		mMsCounter50 = mMsCounter;
	}
	if ( mPingTimer.ellapsed() >= 250 ) {
		uint64_t ticks = Time::GetTick();
		Send( PING, (uint32_t)( ticks & 0xFFFFFFFFL ) );
		uint32_t ret = ReceiveU32();
		uint32_t curr = (uint32_t)( Time::GetTick() & 0xFFFFFFFFL );
		mPing = curr - ret;

		Send( VBAT );
		mBatteryVoltage = ReceiveFloat();
		Send( TOTAL_CURRENT );
		mTotalCurrent = (uint32_t)( ReceiveFloat() * 1000.0f );
		Send( BATTERY_LEVEL );
		mBatteryLevel = ReceiveFloat();

		uint16_t battery_voltage = mADC->Read( 7 );
		if ( battery_voltage != 0 ) {
			mLocalBatteryVoltage = (float)battery_voltage * 5.0f * 2.0f / 4096.0f;
		}

		mPingTimer.Stop();
		mPingTimer.Start();
	}

	bool switch_0 = !digitalRead( 21 );
	bool switch_1 = !digitalRead( 22 );
	bool switch_2 = !digitalRead( 23 );
	bool switch_3 = !digitalRead( 24 );
	bool switch_4 = !digitalRead( 25 );
	if ( switch_0 and not mSwitches[0] ) {
	} else if ( not switch_0 and mSwitches[0] ) {
	}
	if ( switch_1 and not mSwitches[1] ) {
	} else if ( not switch_1 and mSwitches[1] ) {
	}
	if ( switch_2 and not mSwitches[2] ) {
		Arm();
	} else if ( not switch_2 and mSwitches[2] ) {
		Disarm();
	}
	if ( switch_3 and not mSwitches[3] ) {
	} else if ( not switch_3 and mSwitches[3] ) {
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

	float r_thrust = mJoysticks[0].Read();
	float r_yaw = mJoysticks[1].Read();
	float r_pitch = mJoysticks[2].Read();
	float r_roll = mJoysticks[3].Read();
	if ( r_thrust != mThrust and r_thrust >= 0.0f and r_thrust <= 1.0f ) {
		setThrust( r_thrust );
	}
	if ( r_yaw != mControlRPY.z and r_yaw >= -1.0f and r_yaw <= 1.0f ) {
		setYaw( r_yaw );
	}
	if ( r_pitch != mControlRPY.y and r_pitch >= -1.0f and r_pitch <= 1.0f ) {
		setPitch( r_pitch );
	}
	if ( r_roll != mControlRPY.x and r_roll >= -1.0f and r_roll <= 1.0f ) {
		setRoll( r_roll );
	}
	/*
	struct {
		uint32_t cmd;
		uint32_t thrust;
		uint32_t roll;
		uint32_t pitch;
		uint32_t yaw;
	} __attribute__((packed)) cmd_trpy = { htonl( SET_TRPY ), 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	if ( r_thrust >= 0.0f and r_thrust <= 1.0f ) {
		cmd_trpy.thrust = htonf( r_thrust );
	}
	if ( r_yaw >= -1.0f and r_yaw <= 1.0f ) {
		cmd_trpy.yaw = htonf( r_yaw );
	}
	if ( r_pitch >= -1.0f and r_pitch <= 1.0f ) {
		cmd_trpy.pitch = htonf( r_pitch );
	}
	if ( r_roll >= -1.0f and r_roll <= 1.0f ) {
		cmd_trpy.roll = htonf( r_roll );
	}
	mSocket->Send( &cmd_trpy, sizeof( cmd_trpy ) );
	mThrust = ReceiveFloat();
	mControlRPY.x = ReceiveFloat();
	mControlRPY.y = ReceiveFloat();
	mControlRPY.z = ReceiveFloat();
	*/

	mTicks = Time::WaitTick( 1000 / 100, mTicks );
	mMsCounter += ( mTicks - ticks0 );
	return true;
}


void Controller::Calibrate()
{
	if ( !mSocket ) {
		return;
	}

	struct {
		uint32_t cmd = htonl( CALIBRATE );
		uint32_t full_recalibration = 0;
		float current_altitude = 0.0f;
	} calibrate_cmd;
	mSocket->Send( &calibrate_cmd, sizeof( calibrate_cmd ) );
	if ( ReceiveU32() == 0 ) {
		gDebug() << "Calibration success\n";
	}
}


void Controller::CalibrateAll()
{
	if ( !mSocket ) {
		return;
	}

	struct {
		uint32_t cmd = htonl( CALIBRATE );
		uint32_t full_recalibration = 1;
		float current_altitude = 0.0f;
	} calibrate_cmd;
	mSocket->Send( &calibrate_cmd, sizeof( calibrate_cmd ) );
	if ( ReceiveU32() == 0 ) {
		gDebug() << "Calibration success\n";
	}
}


void Controller::Arm()
{
	mXferMutex.lock();
	Send( ARM );
	mArmed = ReceiveU32();
	mXferMutex.unlock();
}


void Controller::Disarm()
{
	mXferMutex.lock();
	Send( DISARM );
	mArmed = ReceiveU32();
	mThrust = 0.0f;
	mXferMutex.unlock();
}


void Controller::ResetBattery()
{
	mXferMutex.lock();
	// TODO
// 	Send( RESET_BATTERY, (uint32_t)mSettings->meta( "battery_mah", 2500 ) );
// 	if ( ReceiveU32() != mSettings->meta( "battery_mah", 2500 ) ) {
// 		gDebug() << "Error while resetting battery state !\n";
// 	}
	mXferMutex.unlock();
}


void Controller::setPID( const Vector3f& v )
{
	mXferMutex.lock();
	Send( SET_PID_P, v.x );
	mPID.x = ReceiveFloat();
	Send( SET_PID_I, v.y );
	mPID.y = ReceiveFloat();
	Send( SET_PID_D, v.z );
	mPID.z = ReceiveFloat();
	mXferMutex.unlock();
}


void Controller::setOuterPID( const Vector3f& v )
{
	mXferMutex.lock();
	Send( SET_OUTER_PID_P, v.x );
	mOuterPID.x = ReceiveFloat();
	Send( SET_OUTER_PID_I, v.y );
	mOuterPID.y = ReceiveFloat();
	Send( SET_OUTER_PID_D, v.z );
	mOuterPID.z = ReceiveFloat();
	mXferMutex.unlock();
}


void Controller::setThrust( const float& v )
{
	mXferMutex.lock();
	Send( SET_THRUST, v );
	mThrust = ReceiveFloat();
	mXferMutex.unlock();
}


void Controller::setThrustRelative( const float& v )
{
	mXferMutex.lock();
	Send( SET_THRUST, mThrust + v );
	mThrust = ReceiveFloat();
	mXferMutex.unlock();
}


void Controller::setRoll( const float& v )
{
	mXferMutex.lock();
	Send( SET_ROLL, v );
	mControlRPY.x = ReceiveFloat();
	mXferMutex.unlock();
}


void Controller::setPitch( const float& v )
{
	mXferMutex.lock();
	Send( SET_PITCH, v );
	mControlRPY.y = ReceiveFloat();
	mXferMutex.unlock();
}



void Controller::setYaw( const float& v )
{
	mXferMutex.lock();
	Send( SET_YAW, v );
	mControlRPY.z = ReceiveFloat();
	mXferMutex.unlock();
}


void Controller::setMode( const Controller::Mode& mode )
{
	mXferMutex.lock();
	Send( SET_MODE, (uint32_t)mode );
	mMode = (Mode)ReceiveU32();
	mXferMutex.unlock();
}


float Controller::acceleration() const
{
	return mAcceleration;
}


const std::list< Vector3f >& Controller::rpyHistory() const
{
	return mRPYHistory;
}


const std::list< Vector3f >& Controller::outerPidHistory() const
{
	return mOuterPIDHistory;
}


float Controller::localBatteryVoltage() const
{
	return mLocalBatteryVoltage;
}


void Controller::Send( const Controller::Cmd& cmd )
{
	if ( !mSocket or !mConnected ) {
		return;
	}

	uint32_t c = htonl( cmd );
	mSocket->Send( &c, sizeof( c ) );
}


void Controller::Send( const Controller::Cmd& cmd, uint32_t v )
{
	if ( !mSocket or !mConnected ) {
		return;
	}

	uint32_t data[2] = { htonl( cmd ), htonl( v ) };
	mSocket->Send( data, sizeof( data ) );
}


void Controller::Send( const Controller::Cmd& cmd, float v )
{
	union {
		float f;
		uint32_t u;
	} u;
	u.f = v;
	Send( cmd, u.u );
}


uint32_t Controller::ReceiveU32()
{
	if ( !mSocket ) {
		return 0;
	}

	uint64_t ticks = Time::GetTick();
	uint32_t ret = 0;
	int err = 0;
	errno = 0;
// 	do {
// 		if ( errno == EAGAIN ) {
// 			gDebug() << "Timeout : " << ( Time::GetTick() - ticks ) << "ms\n";
// 		}
// 		errno = 0;
// 		ticks = Time::GetTick();
		err = mSocket->Receive( &ret, sizeof( ret ), true, 500 );
// 	} while ( err <= 0 and errno == EAGAIN );
	if ( err <= 0/* and errno != EAGAIN*/ ) {
		gDebug() << "error : " << err << " ( " << errno << " ) (timeout : " << ( Time::GetTick() - ticks ) << "\n";
		if ( errno != EAGAIN ) {
			mConnected = false;
		}
		return 0;
	}
	return ntohl( ret );
}


float Controller::ReceiveFloat()
{
	union {
		float f;
		uint32_t u;
	} u;

	u.u = ReceiveU32();
	return u.f;
}
