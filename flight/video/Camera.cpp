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

list<Camera*> Camera::sCameras;

Camera::Camera()
	: mLiveEncoder( nullptr )
	, mVideoEncoder( nullptr )
//	, mStillEncoder( nullptr )
{
	fDebug();
	sCameras.push_back( this );
}


Camera::~Camera()
{
	fDebug();
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
