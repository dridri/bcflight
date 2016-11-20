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
#include <string.h>
#include <sstream>
#include <iostream>
#include "RawWifi.h"
#ifdef WIN32
#include <windows.h>
#else
#include <netinet/in.h>
#endif

// static std::string readcmd( const std::string& cmd, const std::string& entry, const std::string& delim );

std::mutex RawWifi::mInitializingMutex;
bool RawWifi::mInitializing = false;
bool RawWifi::mInitialized = false;

#ifndef NO_RAWWIFI
RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port )
	: Link()
	, mRawWifi( nullptr )
	, mDevice( device )
	, mChannel( 11 )
	, mTxPower( 33 )
	, mCECMode( RAWWIFI_RX_FAST )
	, mOutputPort( out_port )
	, mInputPort( in_port )
	, mRetriesCount( 2 )
	, mLastIsCorrupt( false )
{
}


RawWifi::~RawWifi()
{
}


int RawWifi::setBlocking( bool blocking )
{
	// TODO
	return 0;
}


void RawWifi::setCECMode( const std::string& mode )
{
	if ( mRawWifi ) {
		if ( mode == "weighted" ) {
			rawwifi_set_recv_mode( mRawWifi, RAWWIFI_RX_FEC_WEIGHTED );
			std::cout << "RawWifi " << mDevice << ":" << mChannel << ":inport_port_" << mInputPort << " is now in FEC weighted mode\n";
		} else {
			rawwifi_set_recv_mode( mRawWifi, RAWWIFI_RX_FAST );
		}
	} else {
		if ( mode == "weighted" ) {
			mCECMode = RAWWIFI_RX_FEC_WEIGHTED;
			std::cout << "RawWifi " << mDevice << ":" << mChannel << ":inport_port_" << mInputPort << " is now in FEC weighted mode\n";
		} else {
			mCECMode = RAWWIFI_RX_FAST;
		}
	}
}


void RawWifi::SetChannel( int chan )
{
	mChannel = chan;
}


void RawWifi::SetTxPower( int mBm )
{
	mTxPower = mBm;
}


void RawWifi::setRetriesCount( int retries )
{
	mRetriesCount = retries;
}


int RawWifi::level() const
{
	if ( not mRawWifi ) {
		return 0;
	}
	return mRawWifi->recv_link.signal;
}


int RawWifi::channel() const
{
	return mChannel;
}


bool RawWifi::lastIsCorrupt() const
{
	return mLastIsCorrupt;
}


int32_t RawWifi::RxQuality()
{
	return rawwifi_recv_quality( mRawWifi );
}


void RawWifi::Initialize( const std::string& device, uint32_t channel, uint32_t txpower )
{
	mInitializingMutex.lock();
	while ( mInitializing ) {
		usleep( 1000 * 100 );
	}

	if ( not mInitialized ) {
		mInitialized = true;
		mInitializing = true;

		rawwifi_setup_interface( device.c_str(), channel, txpower, false, 0 );
/*
		std::stringstream ss;

// 		if ( readcmd( "ifconfig " + device + " | grep " + device, "encap", ":" ).find( "UNSPEC" ) == std::string::npos ) {
			ss << "ifconfig " << device << " down";
			ss << " && sleep 0.5 && iw dev " << device << " set monitor otherbss fcsfail";
			ss << " && sleep 0.5 && ifconfig " << device << " up && sleep 0.5 && ";
// 		}
		ss << "iwconfig " << device << " channel " << ( channel - 1 ) << " && sleep 0.5 && iwconfig " << device << " channel " << channel << " && sleep 0.5";
		if ( txpower > 0 ) {
			ss << " && iw dev " << device << " set txpower fixed " << ( txpower * 1000 ) << " && sleep 0.5";
		}

		std::cout << "executing : " << ss.str().c_str() << "\n";
		(void)system( ss.str().c_str() );
*/
		mInitializing = false;
	}
	mInitializingMutex.unlock();
}


int RawWifi::Connect()
{
	Initialize( mDevice, mChannel, mTxPower );

	mRawWifi = rawwifi_init( mDevice.c_str(), mOutputPort, mInputPort, 1, -1 );
	if ( !mRawWifi ) {
		return -1;
	}

	if ( mCECMode != mRawWifi->recv_mode ) {
		rawwifi_set_recv_mode( mRawWifi, mCECMode );
		if ( mCECMode == RAWWIFI_RX_FEC_WEIGHTED ) {
			std::cout << "RawWifi " << mDevice << ":" << mChannel << ":inport_port_" << mInputPort << " is now in FEC weighted mode\n";
		}
	}

	mConnected = true;
	return 0;
}


int RawWifi::Read( void* buf, uint32_t len, int timeout )
{
	if ( !mConnected or mInputPort < 0 ) {
		return -1;
	}

	uint64_t t = GetTicks();
	uint32_t valid = 0;
// 	if ( mInputPort == 11) {
// 		std::cout << "reading...\n";
// 	}
	int ret = rawwifi_recv( mRawWifi, (uint8_t*)buf, len, &valid );
// 	if ( mInputPort == 11) {
// 		std::cout << "read time : " << ( ( GetTicks() - t ) / 1000 ) << "\n";
// 	}

	if ( ret == -3 ) {
		std::cout << "WARNING : Read timeout\n";
		mConnected = false;
		return LINK_ERROR_TIMEOUT;
	}

	if ( ret < 0 ) {
		mConnected = false;
	}
	if ( ret > 0 and not valid ) {
// 		std::cout << "WARNING : Received corrupt packets (port " << mInputPort << ")\n";
		mLastIsCorrupt = true;
	} else {
		mLastIsCorrupt = false;
	}

	if ( ret > 0 ) {
		mReadSpeedCounter += ret;
	}
	if ( GetTicks() - mSpeedTick >= 1000 * 1000 ) {
		mReadSpeed = mReadSpeedCounter;
		mWriteSpeed = mWriteSpeedCounter;
		mReadSpeedCounter = 0;
		mWriteSpeedCounter = 0;
		mSpeedTick = GetTicks();
	}
	return ret;
}


int RawWifi::Write( const void* buf, uint32_t len, int timeout )
{
	if ( !mConnected or mOutputPort < 0 ) {
		return -1;
	}

	int ret = rawwifi_send_retry( mRawWifi, (uint8_t*)buf, len, mRetriesCount );

	if ( ret < 0 ) {
		mConnected = false;
	}

	if ( ret > 0 ) {
		mWriteSpeedCounter += ret;
	}
	if ( GetTicks() - mSpeedTick >= 1000 * 1000 ) {
		mReadSpeed = mReadSpeedCounter;
		mWriteSpeed = mWriteSpeedCounter;
		mReadSpeedCounter = 0;
		mWriteSpeedCounter = 0;
		mSpeedTick = GetTicks();
	}
	return ret;
}

/*
static std::string readcmd( const std::string& cmd, const std::string& entry, const std::string& delim )
{
	char buf[1024] = "";
	std::string res = "";
	FILE* fp = popen( cmd.c_str(), "r" );
	if ( !fp ) {
		printf( "popen failed : %s\n", strerror( errno ) );
		return "";
	}

	if ( entry.length() == 0 or entry == "" ) {
		fread( buf, 1, sizeof( buf ), fp );
		res = buf;
	} else {
		while ( fgets( buf, sizeof(buf), fp ) ) {
			if ( strstr( buf, entry.c_str() ) ) {
				char* s = strstr( buf, delim.c_str() ) + delim.length();
				while ( *s == ' ' or *s == '\t' ) {
					s++;
				}
				char* end = s;
				while ( *end != '\n' and *end++ );
				*end = 0;
				res = std::string( s );
				break;
			}
		}
	}

	pclose( fp );
	return res;
}
*/
#else // NO_RAWWIFI

RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port )
	: Link()
	, mRawWifi( nullptr )
	, mDevice( device )
	, mChannel( 11 )
	, mTxPower( 33 )
	, mCECMode( 0 )
	, mOutputPort( out_port )
	, mInputPort( in_port )
	, mRetriesCount( 2 )
	, mLastIsCorrupt( false )
{
}


RawWifi::~RawWifi()
{
}


int RawWifi::setBlocking( bool blocking )
{
	return -1;
}


void RawWifi::setCECMode( const std::string& mode )
{
}


void RawWifi::SetChannel( int chan )
{
}


void RawWifi::SetTxPower( int mBm )
{
}


void RawWifi::setRetriesCount( int retries )
{
}


int RawWifi::level() const
{
	return -1;
}


int RawWifi::channel() const
{
	return -1;
}


bool RawWifi::lastIsCorrupt() const
{
	return false;
}


int32_t RawWifi::RxQuality()
{
	return 0;
}


void RawWifi::Initialize( const std::string& device, uint32_t channel, uint32_t txpower )
{
}


int RawWifi::Connect()
{
	return -1;
}


int RawWifi::Read( void* buf, uint32_t len, int timeout )
{
	return -1;
}


int RawWifi::Write( const void* buf, uint32_t len, int timeout )
{
	return -1;
}

#endif // NO_RAWWIFI
