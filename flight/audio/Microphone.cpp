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

#include "Debug.h"
#include "Microphone.h"

std::map< std::string, std::function< Microphone* ( Config*, const std::string& ) > > Microphone::mKnownMicrophones;


Microphone::Microphone()
{
}


Microphone::~Microphone()
{
}


Microphone* Microphone::Create( Config* config, const std::string& lua_object )
{
	std::string type = config->string( lua_object + ".type" );

	if ( mKnownMicrophones.find( type ) != mKnownMicrophones.end() ) {
		return mKnownMicrophones[ type ]( config, lua_object );
	}

	gDebug() << "FATAL ERROR : Microphone type \"" << type << "\" not supported !\n";
	return nullptr;
}


void Microphone::RegisterMicrophone( const std::string& name, std::function< Microphone* ( Config*, const std::string& ) > instanciate )
{
	fDebug( name );
	mKnownMicrophones[ name ] = instanciate;
}
