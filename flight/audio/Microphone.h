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

#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <map>
#include "Config.h"

class Microphone
{
public:
	static Microphone* Create( Config* config, const std::string& lua_object );
	Microphone();
	~Microphone();

protected:
	static void RegisterMicrophone( const std::string& name, std::function< Microphone* ( Config*, const std::string& ) > instanciate );
	static std::map< std::string, std::function< Microphone* ( Config*, const std::string& ) > > mKnownMicrophones;
};

#endif // MICROPHONE_H
