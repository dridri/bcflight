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

#ifndef H_CONTROLLER
#define H_CONTROLLER

#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <gammaengine/Time.h>

using namespace GE;

class Controller {
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	static uint32_t ping() { return rand() % 20; }
	static bool armed() { return false; return true; }
	static Mode mode() { return Stabilize; }
	static float thrust() { return std::fmod( Time::GetSeconds() * 0.25f, 1.01f ); }
	static float batteryLevel() { return std::fmod( Time::GetSeconds() * 0.05, 1.0f ); }
	static uint32_t totalCurrent() { return ( Time::GetTick() % 1000000 ) / 1000; }
	static float batteryVoltage() { return 12.6f; }
	static float acceleration() { return 1.0f * 9.8f; return std::fmod( Time::GetSeconds() * 0.47f, 1.0f ) * 10.0f * 9.8f; }
	static Vector3f rpy() { return Vector3f( std::cos( Time::GetSeconds() * 0.74f ) * 15.0f, std::sin( Time::GetSeconds() ) * 110.0f * 0.125f, 0 ); }
};

#endif // H_CONTROLLER
