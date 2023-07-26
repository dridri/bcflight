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

Config* ControllerClient::mConfig = nullptr;

ControllerClient::ControllerClient( Config* config, Link* link, bool spectate )
	: Controller( link, spectate )
	, mADC( nullptr )
	, mSimulatorEnabled( false )
	, mSimulatorSocketServer( nullptr )
	, mSimulatorSocket( nullptr )
	, mSimulatorTicks( 0 )
{
	mConfig = config;

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


float ControllerClient::ReadThrust()
{
	return mJoysticks[0].Read();
}


float ControllerClient::ReadRoll()
{
	return mJoysticks[3].Read();
}


float ControllerClient::ReadPitch()
{
	return mJoysticks[2].Read();
}


float ControllerClient::ReadYaw()
{
	return -mJoysticks[1].Read();
}


bool ControllerClient::run()
{
	if ( mADC == nullptr ) {
		mADC = new MCP320x( mConfig->string( "adc.device", "/dev/spidev1.0" ) );
		if ( mADC ) {
			mADC->setSmoothFactor( 0, 0.5f );
			mADC->setSmoothFactor( 1, 0.5f );
			mADC->setSmoothFactor( 2, 0.5f );
			mADC->setSmoothFactor( 3, 0.5f );
			mADC->setSmoothFactor( 4, 0.75f );
			mJoysticks[0] = Joystick( mADC, 0, 0, true );
			mJoysticks[1] = Joystick( mADC, 1, 1 );
			mJoysticks[2] = Joystick( mADC, 2, 3 );
			mJoysticks[3] = Joystick( mADC, 3, 2 );
		}
	}
	if ( mADC ) {
		uint16_t battery_voltage = mADC->Read( 4 );
		if ( battery_voltage != 0 ) {
			float voltage = (float)battery_voltage * 4.2902f / 1024.0f;
			if ( mLocalBatteryVoltage == 0.0f ) {
				mLocalBatteryVoltage = voltage;
			} else {
				mLocalBatteryVoltage = mLocalBatteryVoltage * 0.98f + voltage * 0.02f;
			}
		}
	}

	return Controller::run();
}


bool ControllerClient::RunSimulator()
{
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
		mSimulatorSocketServer = new rpi::Socket( 5000 );
		mSimulatorSocketServer->Connect();
		std::cout << "mSimulatorSocketServer : " << mSimulatorSocketServer << "\n";
	}
	if ( not mSimulatorSocket ) {
		mSimulatorSocket = mSimulatorSocketServer->WaitClient();
		std::cout << "mSimulatorSocket : " << mSimulatorSocket << "\n";
	}

	float f_thrust = ReadThrust();
	float f_yaw = ReadYaw();
	float f_pitch = ReadPitch();
	float f_roll = ReadRoll();
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

	if ( mSimulatorSocket->Send( &data, sizeof(data) ) <= 0 ) {
		delete mSimulatorSocket;
		mSimulatorSocket = nullptr;
	}

	uint64_t ticks = Thread::GetTick();
	if ( ticks - mSimulatorTicks < 1000 / 100 ) {
		usleep( 1000LLU * std::max( 0LL, 1000LL / 100LL - (int64_t)( ticks - mTicks ) - 1LL ) );
	}
	mSimulatorTicks = Thread::GetTick();
	return true;
}


ControllerClient::Joystick::Joystick( MCP320x* adc, int id, int adc_channel, bool thrust_mode )
	: mADC( adc )
	, mId( id )
	, mADCChannel( adc_channel )
	, mCalibrated( false )
	, mThrustMode( thrust_mode )
	, mMin( 0 )
	, mCenter( 32767 )
	, mMax( 65535 )
{
	mMin = mConfig->setting( "Joystick:" + std::to_string( mId ) + ":min", 0 );
	mCenter = mConfig->setting( "Joystick:" + std::to_string( mId ) + ":cen", 32767 );
	mMax = mConfig->setting( "Joystick:" + std::to_string( mId ) + ":max", 65535 );
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


uint16_t ControllerClient::Joystick::ReadRaw()
{
	if ( mADC == nullptr ) {
		return 0;
	}
	uint32_t raw = mADC->Read( mADCChannel );
	if ( raw != 0 ) {
		mLastRaw = mLastRaw * 0.5f + raw * 0.5f;
	}
	return mLastRaw;
}


float ControllerClient::Joystick::Read()
{
	uint16_t raw = ReadRaw();
	if ( raw == 0 ) {
		return -10.0f;
	}

	if ( mThrustMode ) {
		float ret = (float)( raw - mMin ) / (float)( mMax - mMin );
		ret = 0.01f * std::round( ret * 100.0f );
		ret = std::max( 0.0f, std::min( 1.0f, ret ) );
		return ret;
	}

	float base = mMax - mCenter;
	if ( raw < mCenter ) {
		base = mCenter - mMin;
	}

	float ret = (float)( raw - mCenter ) / base;
	ret = 0.01f * std::round( ret * 100.0f );
	ret = std::max( -1.0f, std::min( 1.0f, ret ) );
//	std::cout << mADCChannel << " : " << ret << "\n";
	return ret;
}


void ControllerClient::SaveThrustCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[0].SetCalibratedValues( min, center, max );
	mConfig->setSetting( "Joystick:" + std::to_string( 0 ) + ":min", min );
	mConfig->setSetting( "Joystick:" + std::to_string( 0 ) + ":cen", center );
	mConfig->setSetting( "Joystick:" + std::to_string( 0 ) + ":max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->SaveSettings( "/root/settings" );
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


void ControllerClient::SaveYawCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[1].SetCalibratedValues( min, center, max );
	mConfig->setSetting( "Joystick:" + std::to_string( 1 ) + ":min", min );
	mConfig->setSetting( "Joystick:" + std::to_string( 1 ) + ":cen", center );
	mConfig->setSetting( "Joystick:" + std::to_string( 1 ) + ":max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->SaveSettings( "/root/settings" );
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


void ControllerClient::SavePitchCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[2].SetCalibratedValues( min, center, max );
	mConfig->setSetting( "Joystick:" + std::to_string( 2 ) + ":min", min );
	mConfig->setSetting( "Joystick:" + std::to_string( 2 ) + ":cen", center );
	mConfig->setSetting( "Joystick:" + std::to_string( 2 ) + ":max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->SaveSettings( "/root/settings" );
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


void ControllerClient::SaveRollCalibration( uint16_t min, uint16_t center, uint16_t max )
{
	mJoysticks[3].SetCalibratedValues( min, center, max );
	mConfig->setSetting( "Joystick:" + std::to_string( 3 ) + ":min", min );
	mConfig->setSetting( "Joystick:" + std::to_string( 3 ) + ":cen", center );
	mConfig->setSetting( "Joystick:" + std::to_string( 3 ) + ":max", max );
	mount( "", "/", "", MS_REMOUNT, nullptr );
	mConfig->SaveSettings( "/root/settings" );
	mount( "", "/", "", MS_REMOUNT | MS_RDONLY, nullptr );
}


bool ControllerClient::SimulatorMode( bool e )
{
	mSimulatorEnabled = e;
	return e;
}
