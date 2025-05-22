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
#include <sys/mount.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pigpio.h>
#include "GPIO.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <Config.h>
#include "ControllerClient.h"
#include "Socket.h"
#include "Debug.h"
#include "PT1.h"

Config* ControllerClient::mConfig = nullptr;

ControllerClient::ControllerClient( Link* link, bool spectate )
	: Controller( link, spectate )
	, mADC( nullptr )
	, mSimulatorEnabled( false )
	, mSimulatorSocketServer( nullptr )
	, mSimulatorSocket( nullptr )
	, mSimulatorTicks( 0 )
{
	mConfig = Config::instance();
	gpioInitialise();

	GPIO::setPUD( 0, GPIO::PullDown );
	GPIO::setPUD( 1, GPIO::PullDown );
	GPIO::setPUD( 4, GPIO::PullDown );
	GPIO::setPUD( 5, GPIO::PullDown );
	GPIO::setPUD( 6, GPIO::PullDown );
	GPIO::setPUD( 12, GPIO::PullDown );
	GPIO::setPUD( 13, GPIO::PullDown );
	GPIO::setPUD( 22, GPIO::PullDown );
	GPIO::setPUD( 23, GPIO::PullDown );
	GPIO::setPUD( 24, GPIO::PullDown );
	GPIO::setPUD( 26, GPIO::PullDown );
	GPIO::setPUD( 27, GPIO::PullDown );

	GPIO::setMode( 0, GPIO::Input );
	GPIO::setMode( 1, GPIO::Input );
	GPIO::setMode( 4, GPIO::Input );
	GPIO::setMode( 5, GPIO::Input );
	GPIO::setMode( 6, GPIO::Input );
	GPIO::setMode( 12, GPIO::Input );
	GPIO::setMode( 13, GPIO::Input );
	GPIO::setMode( 22, GPIO::Input );
	GPIO::setMode( 23, GPIO::Input );
	GPIO::setMode( 24, GPIO::Input );
	GPIO::setMode( 26, GPIO::Input );
	GPIO::setMode( 27, GPIO::Input );

	mSimulatorThread = new HookThread<ControllerClient>( "Simulator", this, &ControllerClient::RunSimulator );
	mSimulatorThread->Start();
}


ControllerClient::~ControllerClient()
{
}


int8_t ControllerClient::ReadSwitch( uint32_t id )
{
	static const uint32_t map[] = { 0, 1, 4, 5, 6, 12, 13, 22, 23, 24, 26, 27 };

	if ( id == 4 ) {
		return 0;
	}
	if ( id >= 12 ) {
		return 0;
	}
	if ( map[id] == 0 ) {
		return 0;
	}

	int8_t ret = !GPIO::Read( map[id] );
	if ( id == 1 ) {
		ret = !ret;
	}
	return ret;
}


float ControllerClient::ReadThrust( float dt )
{
	return mJoysticks[0].Read( dt );
}


float ControllerClient::ReadRoll( float dt )
{
	return mJoysticks[3].Read( dt );
}


float ControllerClient::ReadPitch( float dt )
{
	return mJoysticks[2].Read( dt );
}


float ControllerClient::ReadYaw( float dt )
{
	return mJoysticks[1].Read( dt );
}


bool ControllerClient::run()
{
	if ( mADC == nullptr ) {
		mADC = new MCP320x( mConfig->String( "adc.device", "/dev/spidev1.0" ) );
		if ( mADC ) {
			// mADC->setSmoothFactor( 0, 0.1f );
			// mADC->setSmoothFactor( 1, 0.1f );
			// mADC->setSmoothFactor( 2, 0.1f );
			// mADC->setSmoothFactor( 3, 0.1f );
			// mADC->setSmoothFactor( 4, 0.75f );
			mJoysticks[0] = Joystick( mADC, 1, 0, mConfig->Boolean( "adc.inverse.thrust", false ), true );
			mJoysticks[1] = Joystick( mADC, 2, 1, mConfig->Boolean( "adc.inverse.yaw", false ) );
			mJoysticks[2] = Joystick( mADC, 3, 3, mConfig->Boolean( "adc.inverse.pitch", false ) );
			mJoysticks[3] = Joystick( mADC, 4, 2, mConfig->Boolean( "adc.inverse.roll", false ) );
			mJoysticks[0].setFilter( new PT1_1( 10.0f ) );
			mJoysticks[1].setFilter( new PT1_1( 10.0f ) );
			mJoysticks[2].setFilter( new PT1_1( 10.0f ) );
			mJoysticks[3].setFilter( new PT1_1( 10.0f ) );
		}
	}
	if ( mADC ) {
		uint16_t battery_voltage = mADC->Read( 5, 0.001f );
		if ( battery_voltage != 0 ) {
			// printf( "battery_voltage : %04d\n", battery_voltage );
			constexpr float vref = 5.0f;
			constexpr float R1 = 47000.0f;
			constexpr float R2 = 22000.0f;
			// constexpr float shift = 1.23978f;
			constexpr float shift = 1.1f;
			float voltage = shift * ( (float)battery_voltage * vref ) / ( 4095.0f * ( R2 / ( R1 + R2 ) ) );
			if ( mLocalBatteryVoltage == 0.0f ) {
				mLocalBatteryVoltage = voltage;
			} else if ( voltage > 0.0f ) {
				mLocalBatteryVoltage = mLocalBatteryVoltage * 0.9f + voltage * 0.1f;
			}
		}
	}

	return Controller::run();
}


bool ControllerClient::RunSimulator()
{
	uint64_t ticks = Thread::GetTick();
	if ( ticks - mSimulatorTicks < 1000 / 100 ) {
		usleep( 1000LLU * std::max( 0LL, 1000LL / 100LL - (int64_t)( ticks - mTicks ) - 1LL ) );
	}
	float dt = ((float)( Thread::GetTick() - mSimulatorTicks ) ) / 1000000.0f;
	mSimulatorTicks = Thread::GetTick();

	typedef struct {
		int8_t throttle;
		int8_t roll;
		int8_t pitch;
		int8_t yaw;
	} __attribute__((packed)) ControllerData;
	ControllerData data;

	if ( not mSimulatorEnabled ) {
		if ( mSimulatorSocket ) {
			delete mSimulatorSocket;
		}
		if ( mSimulatorSocketServer ) {
			delete mSimulatorSocketServer;
		}
		mSimulatorSocket = nullptr;
		mSimulatorSocketServer = nullptr;
		usleep( 1000 * 1000 * 2 );
		return true;
	}

	if ( not mSimulatorSocketServer ) {
		mSimulatorSocketServer = new Socket( 5000 );
		mSimulatorSocketServer->Connect();
		std::cout << "mSimulatorSocketServer : " << mSimulatorSocketServer << "\n";
	}
	if ( not mSimulatorSocket ) {
		// mSimulatorSocket = mSimulatorSocketServer->WaitClient();
		mSimulatorSocket = mSimulatorSocketServer;
		std::cout << "mSimulatorSocket : " << mSimulatorSocket << "\n";
	}

	float f_thrust = ReadThrust( dt );
	float f_yaw = ReadYaw( dt );
	float f_pitch = ReadPitch( dt );
	float f_roll = ReadRoll( dt );
	if ( f_thrust >= 0.0f and f_thrust <= 1.0f ) {
		data.throttle = (int8_t)( std::max( 0, std::min( 127, (int32_t)( f_thrust * 127.0f ) ) ) );
	}
	if ( f_yaw >= -1.0f and f_yaw <= 1.0f ) {
		if ( fabsf( f_yaw ) <= 0.025f ) {
			f_yaw = 0.0f;
		} else if ( f_yaw < 0.0f ) {
			f_yaw += 0.025f;
		} else if ( f_yaw > 0.0f ) {
			f_yaw -= 0.025f;
		}
		f_yaw *= 1.0f / ( 1.0f - 0.025f );
		data.yaw = (int8_t)( std::max( -127, std::min( 127, (int32_t)( f_yaw * 127.0f ) ) ) );
	}
	if ( f_pitch >= -1.0f and f_pitch <= 1.0f ) {
		if ( fabsf( f_pitch ) <= 0.025f ) {
			f_pitch = 0.0f;
		} else if ( f_pitch < 0.0f ) {
			f_pitch += 0.025f;
		} else if ( f_pitch > 0.0f ) {
			f_pitch -= 0.025f;
		}
		f_pitch *= 1.0f / ( 1.0f - 0.025f );
		data.pitch = (int8_t)( std::max( -127, std::min( 127, (int32_t)( f_pitch * 127.0f ) ) ) );
	}
	if ( f_roll >= -1.0f and f_roll <= 1.0f ) {
		if ( fabsf( f_roll ) <= 0.025f ) {
			f_roll = 0.0f;
		} else if ( f_roll < 0.0f ) {
			f_roll += 0.025f;
		} else if ( f_roll > 0.0f ) {
			f_roll -= 0.025f;
		}
		f_roll *= 1.0f / ( 1.0f - 0.025f );
		data.roll = (int8_t)( std::max( -127, std::min( 127, (int32_t)( f_roll * 127.0f ) ) ) );
	}

	if ( mSimulatorSocket->Write( &data, sizeof(data) ) <= 0 ) {
		// delete mSimulatorSocket;
		mSimulatorSocket = nullptr;
	}

	return true;
}


ControllerClient::Joystick::Joystick( MCP320x* adc, int id, int adc_channel, bool inverse, bool thrust_mode )
	: mADC( adc )
	, mId( id )
	, mADCChannel( adc_channel )
	, mCalibrated( false )
	, mInverse( inverse )
	, mThrustMode( thrust_mode )
	, mMin( 0 )
	, mCenter( 32767 )
	, mMax( 65535 )
{
	fDebug( adc, id, adc_channel, inverse, thrust_mode );
	mMin = mConfig->Integer( "settings.joysticks[" + std::to_string( mId ) + "].min", 0 );
	mCenter = mConfig->Integer( "settings.joysticks[" + std::to_string( mId ) + "].cen", 32767 );
	mMax = mConfig->Integer( "settings.joysticks[" + std::to_string( mId ) + "].max", 65535 );
	if ( mMin != 0 and mCenter != 32767 and mMax != 65535 ) {
		mCalibrated = true;
	}
}


ControllerClient::Joystick::~Joystick()
{
}


void ControllerClient::Joystick::SetCalibratedValues( uint16_t min, uint16_t center, uint16_t max )
{
	mMin = min;
	mCenter = center;
	mMax = max;
	mCalibrated = true;
}


uint16_t ControllerClient::Joystick::ReadRaw( float dt )
{
	if ( mADC == nullptr ) {
		return 0;
	}
	uint32_t raw = mADC->Read( mADCChannel, dt );
	if ( raw == 0 ) {
		return 0;
	}
	if ( mInverse ) {
		raw = 4096 - raw;
	}
	// if ( raw != 0 ) {
	// 	mLastRaw = mLastRaw * 0.5f + raw * 0.5f;
	// }
	mLastRaw = raw;
	return mLastRaw;
}


float ControllerClient::Joystick::Read( float dt )
{
	uint16_t raw = ReadRaw( dt );
	if ( raw == 0 ) {
		return std::numeric_limits<float>::quiet_NaN();
	}

	if ( mThrustMode ) {
		float ret = (float)( raw - mMin ) / (float)( mMax - mMin );
		ret = 0.001953125f * std::round( ret * 512.0f );
		ret = std::max( 0.0f, std::min( 1.0f, ret ) );
		if ( mFilter ) {
			ret = mFilter->filter( ret, dt );
		}
		return ret;
	}

	float base = mMax - mCenter;
	if ( raw < mCenter ) {
		base = mCenter - mMin;
	}

	float ret = (float)( raw - mCenter ) / base;
		ret = 0.001953125f * std::round( ret * 512.0f );
	ret = std::max( -1.0f, std::min( 1.0f, ret ) );
	if ( mFilter ) {
		ret = mFilter->filter( ret, dt );
	}

	return ret;
}


void ControllerClient::SaveThrustCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[0].SetCalibratedValues( min, center, max );
	mConfig->settings()["joysticks"][1]["min"] = min;
	mConfig->settings()["joysticks"][1]["cen"] = center;
	mConfig->settings()["joysticks"][1]["max"] = max;
	// mConfig->setInteger( "joysticks[" + std::to_string( 1 ) + "].min", min );
	// mConfig->setInteger( "joysticks[" + std::to_string( 1 ) + "].cen", center );
	// mConfig->setInteger( "joysticks[" + std::to_string( 1 ) + "].max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->Save();
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


void ControllerClient::SaveYawCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[1].SetCalibratedValues( min, center, max );
	mConfig->settings()["joysticks"][2]["min"] = min;
	mConfig->settings()["joysticks"][2]["cen"] = center;
	mConfig->settings()["joysticks"][2]["max"] = max;
	// mConfig->setInteger( "joysticks[" + std::to_string( 2 ) + "].min", min );
	// mConfig->setInteger( "joysticks[" + std::to_string( 2 ) + "].cen", center );
	// mConfig->setInteger( "joysticks[" + std::to_string( 2 ) + "].max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->Save();
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


void ControllerClient::SavePitchCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[2].SetCalibratedValues( min, center, max );
	mConfig->settings()["joysticks"][3]["min"] = min;
	mConfig->settings()["joysticks"][3]["cen"] = center;
	mConfig->settings()["joysticks"][3]["max"] = max;
	// mConfig->setInteger( "joysticks[" + std::to_string( 3 ) + "].min", min );
	// mConfig->setInteger( "joysticks[" + std::to_string( 3 ) + "].cen", center );
	// mConfig->setInteger( "joysticks[" + std::to_string( 3 ) + "].max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->Save();
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


void ControllerClient::SaveRollCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[3].SetCalibratedValues( min, center, max );
	mConfig->settings()["joysticks"][4]["min"] = min;
	mConfig->settings()["joysticks"][4]["cen"] = center;
	mConfig->settings()["joysticks"][4]["max"] = max;
	// mConfig->setInteger( "joysticks[" + std::to_string( 4 ) + "].min", min );
	// mConfig->setInteger( "joysticks[" + std::to_string( 4 ) + "].cen", center );
	// mConfig->setInteger( "joysticks[" + std::to_string( 4 ) + "].max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->Save();
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


bool ControllerClient::SimulatorMode( bool e )
{
	mSimulatorEnabled = e;
	return e;
}
