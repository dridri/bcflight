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
#include "Controller.h"
#include "Debug.h"

std::string Debug::sBufferedData;

void Debug::SendControllerOutput( const std::string& s )
{
	sBufferedData += s;

	if ( sBufferedData.find('\n') != sBufferedData.npos and Main::instance() ) {
		Controller* ctrl = Main::instance()->controller();
		if ( ctrl and ctrl->connected() ) {
			ctrl->SendDebug( sBufferedData );
			sBufferedData = "";
		}
	}
}
