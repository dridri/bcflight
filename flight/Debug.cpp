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

#include "Main.h"
#include "Debug.h"
#if ( CONTROLLER == 1)
	#include "Controller.h"
#endif


static string sBufferedData;
static string sBufferedBlackBox;
#ifdef SYSTEM_NAME_Linux
static mutex mMutex;
#endif


void Debug::SendControllerOutput( const string& s )
{
#ifdef SYSTEM_NAME_Linux
	mMutex.lock();
#endif
	sBufferedData += s;

#ifdef SYSTEM_NAME_Linux
	sBufferedBlackBox += s;
	if ( sBufferedBlackBox.find('\n') != sBufferedBlackBox.npos ) {
		if ( sBufferedBlackBox[sBufferedBlackBox.length()-1] == '\n' ) {
			sBufferedBlackBox = sBufferedBlackBox.substr( 0, sBufferedBlackBox.length() - 1 );
		}
		if ( Main::instance() ) {
			Main::instance()->blackbox()->Enqueue( "log", sBufferedBlackBox );
		}
		sBufferedBlackBox = "";
	}
#endif

	if ( sBufferedData.find('\n') != sBufferedData.npos ) {
#if ( CONTROLLER == 1)
		if ( Main::instance() ) {
			Controller* ctrl = Main::instance()->controller();
			if ( ctrl and ctrl->connected() ) {
				ctrl->SendDebug( sBufferedData );
			}
		}
#else
		sBufferedData = "";
#endif
	}
#ifdef SYSTEM_NAME_Linux
	mMutex.unlock();
#endif
}
