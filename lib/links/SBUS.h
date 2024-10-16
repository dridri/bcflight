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

#ifndef SBUS_H
#define SBUS_H

#include "Link.h"
#include "Lua.h"
#include <Bus.h>

class Main;

LUA_CLASS class SBUS : public Link
{
public:
	SBUS( Bus* bus, uint8_t armChannel = 4, uint8_t thrustChannel = 0, uint8_t rollChannel = 1, uint8_t pitchChannel = 2, uint8_t yawChannel = 3 );
	LUA_EXPORT SBUS();
	virtual ~SBUS();

	int Connect();
	int setBlocking( bool blocking );
	void setRetriesCount( int retries );
	int retriesCount() const;
	int32_t Channel();
	int32_t RxQuality();
	int32_t RxLevel();

	SyncReturn Read( void* buf, uint32_t len, int32_t timeout );
	SyncReturn Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 );

	virtual uint32_t fullReadSpeed() { return 0; }

protected:
	static Link* Instanciate( Config* config, const string& lua_object );

	LUA_PROPERTY("bus") Bus* mBus; // TODO : force use Serial @100000bps for now
	int32_t mChannels[32];
	bool mFailsafe;
	LUA_PROPERTY("channels.arm") uint8_t mArmChannel;
	LUA_PROPERTY("channels.thrust") uint8_t mThrustChannel;
	LUA_PROPERTY("channels.roll") uint8_t mRollChannel;
	LUA_PROPERTY("channels.pitch") uint8_t mPitchChannel;
	LUA_PROPERTY("channels.yaw") uint8_t mYawChannel;
	bool mCalibrating;
};
	// string stype = config->String( lua_object + ".device" );
	// int speed = config->Integer( lua_object + ".speed", 100000 );
	// int c_thrust = config->Integer( lua_object + ".channels.thrust", 0 );
	// int c_roll = config->Integer( lua_object + ".channels.roll", 1 );
	// int c_pitch = config->Integer( lua_object + ".channels.pitch", 2 );
	// int c_yaw = config->Integer( lua_object + ".channels.yaw", 3 );
	// int c_arm = config->Integer( lua_object + ".channels.arm", 4 );

#endif // SBUS_H
