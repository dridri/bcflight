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

#ifndef ADS1015_H
#define ADS1015_H

#include <I2C.h>
#include "Voltmeter.h"
#include "CurrentSensor.h"

class ADS1015Channel;

LUA_CLASS class ADS1015 : public Voltmeter
{
public:
	LUA_EXPORT ADS1015();
	~ADS1015();

	void Calibrate( float dt, bool last_pass = false );
	float Read( int channel );

	LUA_EXPORT ADS1015Channel* channel( uint8_t channel ) {
		if ( channel < mChannels.size() ) {
			return mChannels[channel];
		}
		return nullptr;
	}

	LuaValue infos();

protected:
	LUA_PROPERTY("multipliers") Vector4f mMultipliers;
	I2C* mI2C;
	float mRingBuffer[16];
	float mRingSum;
	int mRingIndex;
	bool mChannelReady[4];
	vector<ADS1015Channel*> mChannels;
};

class ADS1015Channel : public Voltmeter {
public:
	void Calibrate( float dt, bool last_pass = false ) {}
	float Read( int channel ) override { (void)channel; return mParent->Read( mChannel ); }
	LuaValue infos() override { return mParent->infos(); }

protected:
	friend class ADS1015;
	ADS1015Channel( ADS1015* parent, uint8_t channel ) : Voltmeter(), mParent(parent), mChannel( channel ) {
		mNames.push_back( "ADS1015.Channel[" + to_string( channel ) + "]" );
	}
	ADS1015* mParent;
	uint8_t mChannel;
};


#endif // ADS1015_H
