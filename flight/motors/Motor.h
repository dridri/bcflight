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

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <string>
#include <map>
#include <functional>

using namespace STD;

class Config;

class Motor
{
public:
	Motor();
	virtual ~Motor();

	const float speed() const;
	void setSpeed( float speed, bool force_hw_update = false );
	void KeepSpeed();
	virtual void Arm() {}
	virtual void Disarm() = 0;
	virtual void Disable() = 0;

	static Motor* Instanciate( const string& name, Config* config, const string& object );
	static void RegisterMotor( const string& name, function< Motor* ( Config*, const string& ) > instanciate );
	static const map< string, function< Motor* ( Config*, const string& ) > > knownMotors() { return mKnownMotors; }

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false ) = 0;

	float mSpeed;

private:
	static map< string, function< Motor* ( Config*, const string& ) > > mKnownMotors;
};

#endif // MOTOR_H
