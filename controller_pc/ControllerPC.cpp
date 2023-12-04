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

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QDebug>
#include "MainWindow.h"
#include "ControllerPC.h"

#ifdef WIN32
#include <windows.h>
#endif

ControllerPC::ControllerPC( Link* link, bool spectate )
	: Controller( link, spectate )
	, mThrust( 0.0f )
	, mArmed( false )
	, mMode( false )
	, mNightMode( false )
	, mRecording( false )
{
#ifdef WIN32
		initGamePad();
#endif

}


ControllerPC::~ControllerPC()
{
}

#ifdef WIN32
void ControllerPC::initGamePad() {
	unsigned int maxGamepadId = joyGetNumDevs();
	JOYINFOEX info;

	for (unsigned int id = 0; id < maxGamepadId; ++id) {
		MMRESULT result = joyGetPosEx(id, &info);
		bool isConnected = result == JOYERR_NOERROR ;
		if (isConnected) {
			qDebug() << "Gamepad detected !";
			padinfo = info;
			gamepadid = id;
			padinfo.dwSize = sizeof(padinfo);
			padinfo.dwFlags = JOY_RETURNALL;
			break;
		}
	}
}
#endif


void ControllerPC::setControlThrust( const float v )
{
	mThrust = v;
}

void ControllerPC::setArmed( const bool armed )
{
	mArmed = armed;
}


void ControllerPC::setModeSwitch( const Controller::Mode& mode )
{
	if ( mode == Controller::Stabilize ) {
		mMode = true;
	} else if ( mode == Controller::Rate ) {
		mMode = false;
	}
}


void ControllerPC::setNight( const bool night )
{
	mNightMode = night;
}


void ControllerPC::setRecording( const bool record )
{
	mRecording = record;
}


float ControllerPC::ReadThrust( float dt )
{
#ifdef WIN32
	if (gamepadid != -1) {
		// down : 18000
		// up : 0
		// Using the POV for the thrust
		joyGetPosEx(gamepadid, &padinfo);
		if (padinfo.dwPOV == 0 && mThrust < 1.0f) {
			mThrust += .001;
		}
		else if (padinfo.dwPOV == 18000 && mThrust > 0.0f) {
			mThrust -= .001;
		}
	}
#endif

	return mThrust;
}


float ControllerPC::ReadRoll( float dt )
{
#ifdef WIN32
	if (gamepadid != -1) {
		joyGetPosEx(gamepadid, &padinfo);
		return ((((float)padinfo.dwXpos / (float)65535)*2) - 1)/2;
	}
#endif
	return 0.0f;
}


float ControllerPC::ReadPitch( float dt )
{
#ifdef WIN32
	if (gamepadid != -1) {
		joyGetPosEx(gamepadid, &padinfo);
		return ((((float)padinfo.dwYpos / (float)65535)*2) - 1)/2;
	}
#endif
	return 0.0f;
}


float ControllerPC::ReadYaw( float dt )
{
#ifdef WIN32
	joyGetPosEx(gamepadid, &padinfo);
	if (gamepadid != -1) {
		joyGetPosEx(gamepadid, &padinfo);
		return ((((float)padinfo.dwRpos / (float)65535)*2) - 1)/2;
	}
#endif
	return 0.0f;
}


int8_t ControllerPC::ReadSwitch( uint32_t id )
{
#ifdef WIN32
	int armBtn = (padinfo.dwButtons & 0x00000002); // Button 2
	if (armBtn) {
		// Security
		mThrust = 0.0f;
		setArmed(true);
	}

	int disArmBtn = (padinfo.dwButtons & 0x00000001); // Button 1
	if (disArmBtn) {
		setArmed(false);
	}
#endif
	if ( id == 9 ) {
		return mArmed;
	}
	if ( id == 5 ) {
		return mMode;
	}
	if ( id == 6 ) {
		return mNightMode;
	}
	if ( id == 10 ) {
		return mRecording;
	}
	return 0;
}


ControllerMonitor::ControllerMonitor( ControllerPC* controller )
	: QThread()
	, mController( controller )
	, mKnowConnected( false )
{
}


ControllerMonitor::~ControllerMonitor()
{
}


void ControllerMonitor::run()
{
	struct sched_param sched;
	memset( &sched, 0, sizeof(sched) );
	sched.sched_priority = sched_get_priority_max( SCHED_RR );
	sched_setscheduler( 0, SCHED_RR, &sched );


	while ( 1 ) {
		if ( not mKnowConnected and mController->isConnected() ) {
			emit connected();
			mKnowConnected = true;
		}

		usleep( 1000 * 50 );
	}
}
