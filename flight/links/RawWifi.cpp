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

#if ( BUILD_RAWWIFI == 1 )

#include <netinet/in.h>
#include <string.h>
#include <Debug.h>
#include "RawWifi.h"
#include "../Config.h"
#include <Board.h>

std::mutex RawWifi::mInitializingMutex;
bool RawWifi::mInitializing = false;
bool RawWifi::mInitialized = false;


int RawWifi::flight_register( Main* main )
{
	RegisterLink( "RawWifi", &RawWifi::Instanciate );
	return 0;
}


Link* RawWifi::Instanciate( Config* config, const std::string& lua_object )
{
	std::string device = config->string( lua_object + ".device", "wlan0" );
	int output_port = config->integer( lua_object + ".output_port" );
	int input_port = config->integer( lua_object + ".input_port" );
	bool blocking = config->boolean( lua_object + ".blocking", true );
	bool drop = config->boolean( lua_object + ".drop", false );

	Link* link = new RawWifi( device, output_port, input_port, blocking, drop );
	static_cast< RawWifi* >( link )->setRetries( config->integer( lua_object + ".retries" ) );
	return link;
}



RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port, bool blocking, bool drop_invalid_packets )
	: Link()
	, mRawWifi( nullptr )
	, mDevice( device )
	, mChannel( 11 )
	, mTxPower( 33 )
	, mOutputPort( out_port )
	, mInputPort( in_port )
	, mBlocking( blocking )
	, mDrop( drop_invalid_packets )
	, mRetries( 2 )
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
	if ( mConnected ) {
		// TODO : reconnect
	}
}


void RawWifi::SetTxPower( int mBm )
{
	mTxPower = mBm;
	if ( mConnected ) {
		// TODO : reconnect
	}
}


void RawWifi::setRetries( int retries )
{
	mRetries = retries;
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
		mInitializing = true;
		mInitialized = true;
		std::stringstream ss;

		(void)system( "ifconfig" );
		usleep( 1000 * 250 );
		(void)system( "iwconfig" );
		usleep( 1000 * 250 );
		if ( Board::readcmd( "ifconfig " + device + " | grep " + device, "encap", ":" ).find( "UNSPEC" ) == std::string::npos ) {
			ss << "ifconfig " << device << " down && sleep 0.5";
			ss << " && iw dev " << device << " set monitor otherbss fcsfail && sleep 0.5";
			ss << " && ifconfig " << device << " up && sleep 0.5 && ";
		}
		ss << "iwconfig " << device << " channel " << ( ( channel + 1 ) % 11 + 1 ) << " && sleep 0.5 && ";
		ss << "iwconfig " << device << " channel " << channel;
		if ( txpower > 0 ) {
			ss << " && iw dev " << device << " set txpower fixed " << ( txpower * 1000 );
		}

		std::cout << "executing : " << ss.str().c_str() << "\n";
		(void)system( ss.str().c_str() );
		usleep( 1000 * 250 );
		(void)system( "ifconfig" );
		usleep( 1000 * 250 );
		(void)system( "iwconfig" );
		mInitializing = false;
	}
	mInitializingMutex.unlock();
}


int RawWifi::Connect()
{
	gDebug() << "1\n";
	Initialize( mDevice, mChannel, mTxPower );
	gDebug() << "2\n";

	mRawWifi = rawwifi_init( "wlan0", mOutputPort, mInputPort, mBlocking );
	gDebug() << "3\n";
	if ( !mRawWifi ) {
		return -1;
	}
	gDebug() << "4\n";

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
		if ( mDrop ) {
			std::cout << "WARNING : Received corrupt packets, dropping\n";
			return 0;
		} else {
			std::cout << "WARNING : Received corrupt packets\n";
		}
	}
	return ret;
}


int RawWifi::Write( const void* buf, uint32_t len, int timeout )
{
	if ( !mConnected or mOutputPort < 0 ) {
		return -1;
	}

	int ret = rawwifi_send_retry( mRawWifi, (uint8_t*)buf, len, mRetries );

	if ( ret < 0 ) {
		mConnected = false;
	}
	return ret;
}

#endif // ( BUILD_RAWWIFI == 1 )
