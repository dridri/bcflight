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

class IMU;
class Controller;
class Config;

class Frame
{
public:
	Frame();
	virtual ~Frame();

	vector< Motor* >* motors();

	virtual void Arm() = 0;
	virtual void Disarm() = 0;
	virtual void WarmUp() = 0;
	virtual bool Stabilize( const Vector3f& pid_output, const float& thrust ) = 0;
	void CalibrateESCs();
	void MotorTest(uint32_t id);

	bool armed() const;
	bool airMode() const;

	static Frame* Instanciate( const string& name, Config* config );
	static void RegisterFrame( const string& name, function< Frame* ( Config* ) > instanciate );
	static const map< string, function< Frame* ( Config* ) > > knownFrames() { return mKnownFrames; }

protected:

	vector< Motor* > mMotors;
	bool mArmed;
	bool mAirMode;

private:
	static map< string, function< Frame* ( Config* ) > > mKnownFrames;
};

#endif // FRAME_H
