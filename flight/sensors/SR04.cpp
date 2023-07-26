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

#include <unistd.h>
#include <Debug.h>
#include <GPIO.h>
#include "SR04.h"
#include "Config.h"


int SR04::flight_register( Main* main )
{
	Device dev;
	dev.iI2CAddr = 0;
	dev.name = "SR04";
	dev.fInstanciate = SR04::Instanciate;
	mKnownDevices.push_back( dev );
	return 0;
}


Sensor* SR04::Instanciate( Config* config, const string& object, Bus* bus )
{
	// TODO : handle bus ?
	SR04* sr04 = new SR04( config->Integer( object + ".gpio_trigger" ), config->Integer( object + ".gpio_echo" ) );
	return sr04;
}


SR04::SR04( uint32_t gpio_trigger, uint32_t gpio_echo )
	: Altimeter()
	, mTriggerPin( gpio_trigger )
	, mEchoPin( gpio_echo )
	, mRiseTick( 0 )
	, mAltitude( 0.0f )
{
	mNames.emplace_back( "SR04" );
	mNames.emplace_back( "sr04" );

	GPIO::setMode( mTriggerPin, GPIO::Output );
	GPIO::setMode( mEchoPin, GPIO::Input );
	GPIO::SetupInterrupt( mEchoPin, GPIO::Both, [this](){ this->Interrupt(); } );
}


SR04::~SR04()
{
}


void SR04::Calibrate( float dt, bool last_pass )
{
}


void SR04::Interrupt()
{
	mISRLock.lock();
	if ( mRiseTick == 0 ) {
		mRiseTick = Board::GetTicks();
	} else {
		uint64_t time = Board::GetTicks() - mRiseTick;
		mRiseTick = 0;
		if ( time > 40000 ) {
			mAltitude = 0.0f;
		} else {
			mAltitude = ( (float)time / 58.0f - 1.0f ) / 100.0f;
		}
	}
	mISRLock.unlock();
}


void SR04::Read( float* altitude )
{
	GPIO::Write( mTriggerPin, true );
	usleep( 12 );
	GPIO::Write( mTriggerPin, false );
	*altitude = mAltitude;
}


LuaValue SR04::infos()
{
	LuaValue ret;

	ret["Trigger Pin"] = mTriggerPin;
	ret["Echo Pin"] = mEchoPin;

	return ret;
}
