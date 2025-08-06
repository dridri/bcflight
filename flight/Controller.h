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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <mutex>
#include <map>
#include <functional>
#include <Thread.h>
#include "Vector.h"
#include "ControllerBase.h"
#include "Lua.h"

using namespace STD;

class Main;
class Link;

LUA_CLASS class Controller : public ControllerBase, public Thread
{
public:
	LUA_EXPORT Controller();
	~Controller();
	Link* link() const;

	void Start();

	bool connected() const;
// 	bool armed() const;
	uint32_t ping() const;
// 	float thrust() const;
// 	const Vector3f& RPY() const;
	bool rpySmoothing() const { return mRPYSmoothingEnabled; }

	LUA_EXPORT void onEvent( ControllerBase::Cmd cmdId, const std::function<void(const LuaValue& v)>& f );

	void Emergency();

	void SendDebug( const string& s );

protected:
	virtual bool run();
	bool TelemetryRun();
	uint32_t crc32( const uint8_t* buf, uint32_t len );

	void Arm();
	void Disarm();
	float setRoll( float value, bool raw = false, float dt = 1.0f / 100.0f );
	float setPitch( float value, bool raw = false, float dt = 1.0f / 100.0f );
	float setYaw( float value, bool raw = false, float dt = 1.0f / 100.0f );
	float setThrust( float value, bool raw = false );
	Main* mMain;
	Link* mClientLink;
#ifdef SYSTEM_NAME_Linux
	mutex mSendMutex;
#endif
	bool mTimedOut;
//	bool mArmed;
	uint32_t mPing;
	LUA_PROPERTY("expo") Vector4f mExpo;
	LUA_PROPERTY("thrust_expo") Vector2f mThrustExpo;
	LUA_PROPERTY("multipliers.thrust") float mThrustMultiplier;
	LUA_PROPERTY("multipliers.roll") float mRollMultiplier;
	LUA_PROPERTY("multipliers.pitch") float mPitchMultiplier;
	LUA_PROPERTY("multipliers.yaw") float mYawMultiplier;
/*
	Vector3f mRPY;
	float mThrust;
	float mThrustAccum;
*/
	LUA_PROPERTY("smoothing") bool mRPYSmoothingEnabled;
	Vector3f mSmoothRPY;
	uint64_t mControlsTicks;
	HookThread< Controller >* mTelemetryThread;
	uint64_t mTelemetryTick;
	uint64_t mTelemetryCounter;
	uint64_t mEmergencyTick;
	LUA_PROPERTY("telemetry_rate") uint32_t mTelemetryFrequency;
	LUA_PROPERTY("full_telemetry") bool mTelemetryFull;
	std::map<uint16_t, std::function<void(const LuaValue& v)>> mEvents;
};

#endif // CONTROLLER_H
