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

#ifndef CONTROLLERPI_H
#define CONTROLLERPI_H

#include "Controller.h"
#include <Link.h>
#include <MCP320x.h>
#include "Filter.h"
#include <Lua.h>

class Socket;

LUA_CLASS class ControllerClient : public Controller
{
public:
	LUA_EXPORT ControllerClient( Link* link, bool spectate = false );
	~ControllerClient();

	class Joystick {
	public:
		Joystick()
			: mADC( nullptr ),
			mADCChannel( 0 ),
			mCalibrated( false ),
			mInverse( false ),
			mThrustMode( false ),
			mMin( 0 ),
			mCenter( 0 ),
			mMax( 0 ),
			mLastRaw( 0 ),
			mFilter( nullptr )
		{}
		Joystick( MCP320x* adc, int id, int channel, bool inverse = false, bool thrust_mode = false );
		~Joystick();
		uint16_t ReadRaw( float dt );
		uint16_t LastRaw() const { return mLastRaw; }
		float Read( float dt );
		void SetCalibratedValues( uint16_t min, uint16_t center, uint16_t max );
		void setFilter( Filter<float>* filter ) { mFilter = filter; }
		uint16_t max() const { return mMax; }
		uint16_t center() const { return mCenter; }
		uint16_t min() const { return mMin; }
	private:
		MCP320x* mADC;
		int mId;
		int mADCChannel;
		bool mCalibrated;
		bool mInverse;
		bool mThrustMode;
		uint16_t mMin;
		uint16_t mCenter;
		uint16_t mMax;
		uint16_t mLastRaw;
		Filter<float>* mFilter;
	};

	Joystick* joystick( int x ) { return &mJoysticks[x]; }
	uint16_t rawThrust( float dt ) { return mJoysticks[0].ReadRaw( dt ); }
	uint16_t rawYaw( float dt ) { return mJoysticks[1].ReadRaw( dt ); }
	uint16_t rawRoll( float dt ) { return mJoysticks[3].ReadRaw( dt ); }
	uint16_t rawPitch( float dt ) { return mJoysticks[2].ReadRaw( dt ); }
	void SaveThrustCalibration( uint16_t min, uint16_t center, uint16_t max );
	void SaveYawCalibration( uint16_t min, uint16_t center, uint16_t max );
	void SavePitchCalibration( uint16_t min, uint16_t center, uint16_t max );
	void SaveRollCalibration( uint16_t min, uint16_t center, uint16_t max );
	bool SimulatorMode( bool enabled );

protected:
	virtual bool run();
	bool RunSimulator();

	float ReadThrust( float dt );
	float ReadRoll( float dt );
	float ReadPitch( float dt );
	float ReadYaw( float dt );
	int8_t ReadSwitch( uint32_t id );

	static Config* mConfig;
	MCP320x* mADC;
	Joystick mJoysticks[4];
	bool mSimulatorEnabled;
	HookThread<ControllerClient>* mSimulatorThread;
	Socket* mSimulatorSocketServer;
	Socket* mSimulatorSocket;
	uint64_t mSimulatorTicks;
};

#endif // CONTROLLERPI_H
