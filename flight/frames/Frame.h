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

#ifndef FRAME_H
#define FRAME_H

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <stdint.h>
#include <Motor.h>
#include <Vector.h>
#include <Thread.h>

class IMU;
class Controller;
class Config;

class Frame
{
public:
	Frame();
	virtual ~Frame();

	vector< Motor* >* motors();

	virtual void Arm();
	virtual void Disarm() = 0;
	virtual void WarmUp() = 0;
	virtual bool Stabilize( const Vector3f& pid_output, float thrust, float dt = 0.0f ) = 0;

	void CalibrateESCs();
	void MotorTest( uint32_t id );
	void MotorsBeep( bool enabled );

	bool armed() const;
	bool airMode() const;

	static void RegisterFrame( const string& name, function< Frame* ( Config* ) > instanciate );
	static const map< string, function< Frame* ( Config* ) > > knownFrames() { return mKnownFrames; }

protected:
	LUA_PROPERTY("motors") vector< Motor* > mMotors;
	bool mArmed;
	bool mAirMode;
	bool mMotorsBeep;
	uint8_t mMotorBeepMode;

	HookThread< Frame >* mMotorsBeepThread;

	bool MotorsBeepRun();

private:
	static map< string, function< Frame* ( Config* ) > > mKnownFrames;
};

#endif // FRAME_H
