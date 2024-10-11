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
#include <Debug.h>
#include "Frame.h"
#include <Config.h>

map< string, function< Frame* ( Config* ) > > Frame::mKnownFrames;

Frame::Frame()
	: mMotors( vector< Motor* >() )
	, mArmed( false )
	, mAirMode( false )
	, mMotorsBeep( false )
	, mMotorBeepMode( 0 )
	, mMotorsBeepThread( new HookThread< Frame >( "motors_beep", this, &Frame::MotorsBeepRun ) )
{
	mMotorsBeepThread->setFrequency( 1 );
}


Frame::~Frame()
{
}


void Frame::MotorTest( uint32_t id )
{
	gDebug() << "Test motor : " << id;
	if (mMotors.size() <= id) {
		gDebug() << "Out of range abort";
	}
	
	Motor* m = mMotors[id];
	m->setSpeed( 0.0f, true );
	usleep(1 * 1000 * 1000);
	m->Disarm();
	usleep(1 * 1000 * 1000);
	m->setSpeed( 0.0f, true );
	usleep(1 * 1000 * 1000);
	m->setSpeed(0.05f, true);
	gDebug() << "Waiting for 5 seconds";
	usleep(5 * 1000 * 1000);
	m->Disarm();
	gDebug() << "Disarm ESC";
}


void Frame::MotorsBeep( bool enabled )
{
	mMotorsBeep = enabled;
	if ( enabled and not mMotorsBeepThread->running() ) {
		mMotorBeepMode = 0;
		mMotorsBeepThread->Start();
	} else if ( not enabled and mMotorsBeepThread->running() ) {
		mMotorsBeepThread->Stop();
	}
}


bool Frame::MotorsBeepRun()
{
	for ( Motor* m : mMotors ) {
		m->Beep( mMotorBeepMode );
	}

	usleep( 260 * 1000 );
	for ( Motor* m : mMotors ) {
		m->Disarm();
	}

	usleep( 1000 * 1000 );
	mMotorBeepMode = ( mMotorBeepMode + 1 ) % 2;
	return true;
}


void Frame::CalibrateESCs()
{
	fDebug();

	gDebug() << "Disabling ESCs";
	for ( Motor* m : mMotors ) {
		m->Disable();
	}

	gDebug() << "Waiting 10 seconds...";
	usleep( 10 * 1000 * 1000 );

	gDebug() << "Setting maximal speed";
	for ( Motor* m : mMotors ) {
		m->setSpeed( 1.0f, true );
	}

	gDebug() << "Waiting 8 seconds...";
	usleep( 8 * 1000 * 1000 );

	gDebug() << "Setting minimal speed";
	for ( Motor* m : mMotors ) {
		m->setSpeed( 0.0f, true );
	}

	gDebug() << "Waiting 8 seconds...";
	usleep( 8 * 1000 * 1000 );

	gDebug() << "Disarm all ESCs";
	for ( Motor* m : mMotors ) {
		m->Disarm();
	}
}


void Frame::Arm()
{
	if ( mMotorsBeep ) {
		mMotorsBeepThread->Stop();
		mMotorsBeepThread->Join();
		mMotorsBeep = false;
		usleep( 500 * 1000 );
	}
}


bool Frame::armed() const
{
	return mArmed;
}


bool Frame::airMode() const
{
	return mAirMode;
}


vector< Motor* >* Frame::motors()
{
	return &mMotors;
}


void Frame::RegisterFrame( const string& name, function< Frame* ( Config* ) > instanciate )
{
	mKnownFrames[ name ] = instanciate;
}
