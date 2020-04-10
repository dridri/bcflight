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

#include <cmath>
#include "Motor.h"

map< string, function< Motor* ( Config*, const string& ) > > Motor::mKnownMotors;

Motor::Motor()
	: mSpeed( -1 )
{
}


Motor::~Motor()
{
}


const float Motor::speed() const
{
	return mSpeed;
}


void Motor::KeepSpeed()
{
	setSpeed( mSpeed );
}


void Motor::setSpeed( float speed, bool force_hw_update )
{
	if ( speed == mSpeed and not force_hw_update ) {
		return;
	}
/*
	uint8_t uspeed = (int)( fmaxf( 0, fminf( 255, (int)( speed * 255.0f ) ) ) );
	setSpeedRaw( uspeed );
*/
	setSpeedRaw( speed, force_hw_update );
	mSpeed = speed;
}


Motor* Motor::Instanciate( const string& name, Config* config, const string& object )
{
	if ( mKnownMotors.find( name ) != mKnownMotors.end() ) {
		return mKnownMotors[ name ]( config, object );
	}
	return nullptr;
}


void Motor::RegisterMotor( const string& name, function< Motor* ( Config*, const string& ) > instanciate )
{
	mKnownMotors[ name ] = instanciate;
}
