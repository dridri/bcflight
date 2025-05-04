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
#include "Serial.h"

class Main;

class SBUS : public Link
{
public:
	SBUS( const string& device, int speed = 100000, uint8_t armChannel = 4, uint8_t thrustChannel = 0, uint8_t rollChannel = 1, uint8_t pitchChannel = 2, uint8_t yawChannel = 3 );
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

	static int flight_register( Main* main );

protected:
	static Link* Instanciate( Config* config, const string& lua_object );

	Serial* mSerial;
// 	vector<uint8_t> mBuffer;
	int32_t mChannels[32];
	bool mFailsafe;
	uint8_t mArmChannel;
	uint8_t mThrustChannel;
	uint8_t mRollChannel;
	uint8_t mPitchChannel;
	uint8_t mYawChannel;
	bool mCalibrating;
};

#endif // SBUS_H
