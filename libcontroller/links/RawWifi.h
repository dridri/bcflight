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

#ifndef RAWWIFI_H
#define RAWWIFI_H

//#ifndef NO_RAWWIFI

#include "Link.h"
#include <mutex>
#ifdef NO_RAWWIFI
typedef void rawwifi_t;
typedef enum {
	RAWWIFI_RX_FAST = 0,
	RAWWIFI_RX_FEC_WEIGHTED = 1,
	RAWWIFI_RX_FEC_CEC = 2
} RAWWIFI_RX_FEC_MODE;

typedef enum {
	RAWWIFI_FILL_WITH_ZEROS = 0, // default : fill missing parts of blocks with zeros
	RAWWIFI_CONTIGUOUS = 1,  // put all the packets one behind the other
} RAWWIFI_BLOCK_RECOVER_MODE;
#else
#include <rawwifi.h>
#endif

class RawWifi : public Link
{
public:
	RawWifi( const std::string& device, int16_t out_port, int16_t in_port = -1, int read_timeout_ms = -1 );
	~RawWifi();

	int Connect();
	int32_t Channel();
	int32_t RxQuality();
	int32_t RxLevel();

	void SetChannel( int chan );
	void SetTxPower( int dBm );
	int setBlocking( bool blocking );
	void setCECMode( const std::string& mode );
	void setBlockRecoverMode( const std::string& mode );
	void setRetriesCount( int retries );

	int level() const;
	int channel() const;
	bool lastIsCorrupt() const;
	int retriesCount() const;

	virtual uint32_t fullReadSpeed();

protected:
	static void Initialize( const std::string& device, uint32_t channel, uint32_t txpower );
	int Read( void* buf, uint32_t len, int32_t timeout );
	int Write( const void* buf, uint32_t len, int32_t timeout );

	rawwifi_t* mRawWifi;
	std::string mDevice;
	int mChannel;
	int mTxPower;
	uint32_t mReadTimeout;
	RAWWIFI_RX_FEC_MODE mCECMode;
	RAWWIFI_BLOCK_RECOVER_MODE mRecoverMode;
	int16_t mOutputPort;
	int16_t mInputPort;
	int mRetriesCount;
	bool mLastIsCorrupt;

	static std::mutex mInitializingMutex;
	static bool mInitializing;
	static bool mInitialized;
};

//#endif // NO_RAWWIFI

#endif // RAWWIFI_H
