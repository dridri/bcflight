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
#include <Thread.h>
#include "Vector.h"
#include "ControllerBase.h"

using namespace STD;

class Main;
class Link;

class Controller : public ControllerBase, public Thread
{
public:
	Controller( Main* main, Link* link );
	~Controller();
	Link* link() const;

	bool connected() const;
	bool armed() const;
	uint32_t ping() const;
	float thrust() const;
	const Vector3f& RPY() const;

	void UpdateSmoothControl( const float& dt );
	void Emergency();

	void SendDebug( const string& s );

protected:
	virtual bool run();
	bool TelemetryRun();
	uint32_t crc32( const uint8_t* buf, uint32_t len );

	void Arm();
	void Disarm();
	void setRoll( float value );
	void setPitch( float value );
	void setYaw( float value );
	void setThrust( float value );

	Main* mMain;
#ifdef SYSTEM_NAME_Linux
	mutex mSendMutex;
#endif
	bool mTimedOut;
	bool mArmed;
	uint32_t mPing;
	Vector4f mExpo;
	Vector3f mRPY;
	float mThrust;
	float mThrustAccum;
	Vector3f mSmoothRPY;
	uint64_t mTicks;
	HookThread< Controller >* mTelemetryThread;
	uint64_t mTelemetryTick;
	uint64_t mTelemetryCounter;
	uint64_t mEmergencyTick;
	uint32_t mTelemetryFrequency;
	bool mTelemetryFull;
};

#endif // CONTROLLER_H
