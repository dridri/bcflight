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
#include "MainWindow.h"
#include "ControllerPC.h"

ControllerPC::ControllerPC( Link* link )
	: Controller( link )
	, mThrust( 0.0f )
	, mArmed( false )
{
}


ControllerPC::~ControllerPC()
{
}


void ControllerPC::setControlThrust( const float v )
{
	mThrust = v;
}


void ControllerPC::setArmed( const bool armed )
{
	mArmed = armed;
}


float ControllerPC::ReadThrust()
{
	return mThrust;
}


float ControllerPC::ReadRoll()
{
	return 0.0f;
}


float ControllerPC::ReadPitch()
{
	return 0.0f;
}


float ControllerPC::ReadYaw()
{
	return 0.0f;
}


int8_t ControllerPC::ReadSwitch( uint32_t id )
{
	if ( id == 2 ) {
		return mArmed;
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
		usleep( 1000 * 100 );
	}
}
