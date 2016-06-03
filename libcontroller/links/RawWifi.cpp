#ifndef NO_RAWWIFI

#include <netinet/in.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include "RawWifi.h"

static std::string readcmd( const std::string& cmd, const std::string& entry, const std::string& delim );

bool RawWifi::mInitialized = false;

RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port )
	: Link()
	, mRawWifi( nullptr )
	, mDevice( device )
	, mChannel( 9 )
	, mTxPower( 33 )
	, mOutputPort( out_port )
	, mInputPort( in_port )
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
			ss << " && iw dev " << device << " set monitor otherbss fcsfail";
			ss << " && ifconfig " << device << " up && ";
// 		}
		ss << "iwconfig " << device << " channel " << channel;
		if ( txpower > 0 ) {
			ss << " && iw dev " << device << " set txpower fixed " << ( txpower * 1000 );
		}

		std::cout << "executing : " << ss.str().c_str() << "\n";
		(void)system( ss.str().c_str() );
	}
}


int RawWifi::Connect()
{
	std::stringstream ss;

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

	int ret = rawwifi_send( mRawWifi, (uint8_t*)buf, len );

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

#endif // NO_RAWWIFI
