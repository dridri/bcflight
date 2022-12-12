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

map< string, function< Microphone* ( Config*, const string& ) > > Microphone::mKnownMicrophones;


Microphone::Microphone()
{
}


Microphone::~Microphone()
{
}


Microphone* Microphone::Create( Config* config, const string& lua_object )
{
	string type = config->String( lua_object + ".type" );

	if ( mKnownMicrophones.find( type ) != mKnownMicrophones.end() ) {
		return mKnownMicrophones[ type ]( config, lua_object );
	}

	gDebug() << "FATAL ERROR : Microphone type \"" << type << "\" not supported !";
	return nullptr;
}


void Microphone::RegisterMicrophone( const string& name, function< Microphone* ( Config*, const string& ) > instanciate )
{
	fDebug( name );
	mKnownMicrophones[ name ] = instanciate;
}
