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

#include <Controller.h>
#include <Link.h>
#include "../../ADCs/MCP320x.h"

class ControllerClient : public Controller
{
public:
	ControllerClient( Config* config, Link* link, bool spectate = false );
	~ControllerClient();

	class Joystick {
	public:
		Joystick() : mADC( nullptr ), mADCChannel( 0 ), mCalibrated( false ), mThrustMode( false ), mMin( 0 ), mCenter( 0 ), mMax( 0 ), mLastRaw( 0 ) {}
		Joystick( MCP320x* adc, int id, int channel, bool thrust_mode = false );
		~Joystick();
		uint16_t ReadRaw();
		uint16_t LastRaw() const { return mLastRaw; }
		float Read();
		void SetCalibratedValues( uint16_t min, uint16_t center, uint16_t max );
		uint16_t max() const { return mMax; }
		uint16_t center() const { return mCenter; }
		uint16_t min() const { return mMin; }
	private:
		MCP320x* mADC;
		int mId;
		int mADCChannel;
		bool mCalibrated;
		bool mThrustMode;
		uint16_t mMin;
		uint16_t mCenter;
		uint16_t mMax;
		uint16_t mLastRaw;
	};

	Joystick* joystick( int x ) { return &mJoysticks[x]; }

protected:
	virtual bool run();

	float ReadThrust();
	float ReadRoll();
	float ReadPitch();
	float ReadYaw();
	int8_t ReadSwitch( uint32_t id );

	Config* mConfig;
	MCP320x* mADC;
	Joystick mJoysticks[4];
};

#endif // CONTROLLERPI_H
