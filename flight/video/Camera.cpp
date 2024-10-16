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

#include <Debug.h>
#include "Camera.h"
#include "Main.h"
#include "IMU.h"
#include "Servo.h"

list<Camera*> Camera::sCameras;

Camera::Camera()
	: mLiveEncoder( nullptr )
	, mVideoEncoder( nullptr )
//	, mStillEncoder( nullptr )
	, mTiltMotor( nullptr )
	, mPanMotor( nullptr )
	, mTiltMode( FIXED )
	, mPanMode( FIXED )
	, mTiltOffset( 0.0f )
	, mPanOffset( 0.0f )
	, mTiltRatio( 1.0f )
	, mPanRatio( 1.0f )
	, mPTZThread( nullptr )
{
	fDebug();
	sCameras.push_back( this );
}


Camera::~Camera()
{
	fDebug();
}


void Camera::Start()
{
	if ( ( mTiltMotor or mPanMotor ) and not mPTZThread ) {
		mPTZThread = new HookThread<Camera>( "CameraPTZ", this, &Camera::PTZThreadRun );
	}
	if ( mPTZThread ) {
		mPTZThread->Start();
	}
}


bool Camera::PTZThreadRun()
{
	if ( mTiltMode == FIXED and mPanMode == FIXED ) {
		if ( mTiltMotor ) {
			mTiltMotor->setValue( mTiltOffset * mTiltRatio );
		}
		if ( mPanMotor ) {
			mPanMotor->setValue( mPanOffset * mPanRatio );
		}
		mPTZThread->setFrequency( 1 );
		return true;
	}

	if ( mPTZThread->frequency() == 1 ) {
		mPTZThread->setFrequency( 60 );
	}

	Vector3f rpy = Main::instance()->imu()->RPY();

	if ( mTiltMode == DYNAMIC and mTiltMotor ) {
		mTiltMotor->setValue( ( rpy.y + mTiltOffset ) * mTiltRatio );
	}

	if ( mPanMode == DYNAMIC and mPanMotor ) {
		mPanMotor->setValue( ( rpy.z + mPanOffset ) * mPanRatio );
	}

	return true;
}


LuaValue Camera::infosAll()
{
	LuaValue ret;

	uint32_t i = 0;
	for ( auto& c : sCameras ) {
		ret["Camera " + to_string( i )] = c->infos();
	}

	return ret;
}
