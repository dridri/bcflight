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

#ifndef NO_RAWWIFI

#include <netinet/in.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include "RawWifi.h"

// static std::string readcmd( const std::string& cmd, const std::string& entry, const std::string& delim );

bool RawWifi::mInitialized = false;

RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port )
	: Link()
	, mRawWifi( nullptr )
	, mDevice( device )
	, mChannel( 11 )
	, mTxPower( 33 )
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


void RawWifi::SetChannel( int chan )
{
	mChannel = chan;
	if ( mConnected and mChannel > 0 and mChannel < 15 ) {
		std::stringstream ss;
		ss << "iwconfig " << mDevice << " channel " << mChannel;
		(void)system( ss.str().c_str() );
	}
}


void RawWifi::SetTxPower( int mBm )
{
	mTxPower = mBm;
	if ( mConnected and mTxPower > 0 ) {
		std::stringstream ss;
		ss << "iw dev " << mDevice << " set txpower fixed " << ( mTxPower * 1000 );
		(void)system( ss.str().c_str() );
	}
}


void RawWifi::setRetriesCount( int retries )
{
	mRetriesCount = retries;
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
	if ( not mInitialized ) {
		mInitialized = true;
		std::stringstream ss;

// 		if ( readcmd( "ifconfig " + device + " | grep " + device, "encap", ":" ).find( "UNSPEC" ) == std::string::npos ) {
			ss << "ifconfig " << device << " down";
			ss << " && sleep 0.5 && iw dev " << device << " set monitor otherbss fcsfail";
			ss << " && sleep 0.5 && ifconfig " << device << " up && sleep 0.5 && ";
// 		}
		ss << "iwconfig " << device << " channel " << ( channel - 1 ) << " && sleep 0.5 && iwconfig " << device << " channel " << channel;
		if ( txpower > 0 ) {
			ss << " && iw dev " << device << " set txpower fixed " << ( txpower * 1000 );
		}

		std::cout << "executing : " << ss.str().c_str() << "\n";
		(void)system( ss.str().c_str() );
	}
}


int RawWifi::Connect()
{
	Initialize( mDevice, mChannel, mTxPower );

	mRawWifi = rawwifi_init( mDevice.c_str(), mOutputPort, mInputPort, 1 );
	if ( !mRawWifi ) {
		return -1;
	}

	mConnected = true;
	return 0;
}


int RawWifi::Read( void* buf, uint32_t len, int timeout )
{
	if ( !mConnected or mInputPort < 0 ) {
		return -1;
	}

	uint32_t valid = 0;
	int ret = rawwifi_recv( mRawWifi, (uint8_t*)buf, len, &valid );

	if ( ret < 0 ) {
		mConnected = false;
	}
	if ( ret > 0 and not valid ) {
		std::cout << "WARNING : Received corrupt packets\n";
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
#endif // NO_RAWWIFI
