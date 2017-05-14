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
#include <mutex>
#include "Link.h"

#if ( BUILD_RAWWIFI == 1 )

#include <list>
#include "../../librawwifi/rawwifi.h"
class Main;

class RawWifi : public Link
{
public:
	RawWifi( const std::string& device, int16_t out_port, int16_t in_port = -1, int read_timeout_ms = -1, bool blocking = true, bool drop_invalid_packets = false );
	~RawWifi();

	int Connect();

	void SetChannel( int chan );
	void SetTxPower( int dBm );
	int setBlocking( bool blocking );
	void setCECMode( const std::string& mode );
	void setBlockRecoverMode( const std::string& mode );
	void setRetriesCount( int retries );
	void setMaxBlockSize( int max );
	void setTXFlags( RAWWIFI_BLOCK_FLAGS flags );

	int32_t Channel();
	int32_t Frequency();
	int32_t RxQuality();
	int32_t RxLevel();
	uint32_t TXHeadersSize();
	RAWWIFI_BLOCK_FLAGS TXFlags();
	int retriesCount() const { return mRetries; }
	virtual uint32_t fullReadSpeed();

	int Read( void* buf, uint32_t len, int32_t timeout );
	int Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 );

	static int flight_register( Main* main );

protected:
	static void Initialize( const std::string& device, uint32_t channel, uint32_t txpower );
	static Link* Instanciate( Config* config, const std::string& lua_object );

	rawwifi_t* mRawWifi;
	std::string mDevice;
	int mReadTimeout;
	int mMaxBlockSize;
	int mChannel;
	int mTxPower;
	int16_t mOutputPort;
	int16_t mInputPort;
	bool mBlocking;
	bool mDrop;
	uint32_t mRetries;
	RAWWIFI_BLOCK_FLAGS mSendFlags;

	static std::mutex mInitializingMutex;
	static bool mInitializing;
	static std::list<std::string> mInitialized;
	static std::map<std::string, std::list<int16_t> > mUsedPorts;
};

#else

class RawWifi : public Link
{
public:
	RawWifi( const std::string& device, int16_t out_port, int16_t in_port = -1 ) {}
	~RawWifi() {}
	static int flight_register( Main* main ){ return 0; }
};

#endif // ( BUILD_RAWWIFI == 1 )

#endif // RAWWIFI_H
