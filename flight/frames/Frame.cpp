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

std::map< std::string, std::function< Frame* ( Config* ) > > Frame::mKnownFrames;

Frame::Frame()
	: mMotors( std::vector< Motor* >() )
	, mArmed( false )
	, mAirMode( false )
{
}


Frame::~Frame()
{
}
void Frame::MotorTest(uint32_t id) {
	
	gDebug() << "Test motor : " << id << "\n";
	if (mMotors.size() <= id) {
		gDebug() << "Out of range abort\n";
	}
	
	Motor *m = mMotors[id];
	m->setSpeed( 0.0f, true );
	usleep(1 * 1000 * 1000);
	m->Disarm();
	usleep(1 * 1000 * 1000);
	m->setSpeed( 0.0f, true );
	usleep(1 * 1000 * 1000);
	m->setSpeed(0.05f, true);
	gDebug() << "Waiting for 5 seconds\n";
	usleep(5 * 1000 * 1000);
	m->Disarm();
	gDebug() << "Disarm ESC\n";
}

void Frame::CalibrateESCs()
{
	fDebug0();

	gDebug() << "Disabling ESCs\n";
	for ( Motor* m : mMotors ) {
		m->Disable();
	}

	gDebug() << "Waiting 10 seconds...\n";
	usleep( 10 * 1000 * 1000 );

	gDebug() << "Setting maximal speed\n";
	for ( Motor* m : mMotors ) {
		m->setSpeed( 1.0f, true );
	}

	gDebug() << "Waiting 8 seconds...\n";
	usleep( 8 * 1000 * 1000 );

	gDebug() << "Setting minimal speed\n";
	for ( Motor* m : mMotors ) {
		m->setSpeed( 0.0f, true );
	}

	gDebug() << "Waiting 8 seconds...\n";
	usleep( 8 * 1000 * 1000 );

	gDebug() << "Disarm all ESCs\n";
	for ( Motor* m : mMotors ) {
		m->Disarm();
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


Frame* Frame::Instanciate( const std::string& name, Config* config )
{
	if ( mKnownFrames.find( name ) != mKnownFrames.end() ) {
		return mKnownFrames[ name ]( config );
	}
	return nullptr;
}


const std::vector< Motor* >& Frame::motors() const
{
	return mMotors;
}


void Frame::RegisterFrame( const std::string& name, std::function< Frame* ( Config* ) > instanciate )
{
	mKnownFrames[ name ] = instanciate;
}
