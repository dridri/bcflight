#if ( BUILD_RAWWIFI == 1 )

#include <netinet/in.h>
#include <string.h>
#include <Debug.h>
#include "RawWifi.h"
#include "../Config.h"


RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port )
	: Link()
	, mRaWifi( nullptr )
	, mDevice( device )
	, mChannel( 13 )
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


int RawWifi::Connect()
{
	std::stringstream ss;

	ss << "ifconfig " << mDevice << " down";
	ss << " && iw dev " << mDevice << " set monitor otherbss fcsfail";
	ss << " && ifconfig " << mDevice << " up";
	ss << " && iwconfig " << mDevice << " channel " << mChannel;
	ss << " && iw dev " << mDevice << " set bitrates mcs-2.4 3"; // MCS-3 == 26Mbps
	ss << " && iwconfig " << mDevice << " rate 26M";
	if ( mTxPower > 0 ) {
		ss << " && iw dev " << mDevice << " set txpower fixed " << ( mTxPower * 1000 );
	}
	gDebug() << "executing : " << ss.str().c_str() << "\n";
	(void)system( ss.str().c_str() );

	char* mode = "";
	if ( mOutputPort >= 0 and mInputPort >= 0 ) {
		mode = "rw";
	} else if ( mOutputPort >= 0 ) {
		mode = "w";
	} else if ( mInputPort >= 0 ) {
		mode = "r";
	}
	if ( mOutputPort >= 0 and mInputPort >= 0 and mOutputPort != mInputPort ) {
		gDebug() << "output port and input port should be the same !\n";
		return -1;
	}

	mRaWifi = rawwifi_init( "wlan0", mode, mOutputPort, 1 );
	if ( !mRaWifi ) {
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

	int ret = rawwifi_recv( mRaWifi, (uint8_t*)buf, len );

	if ( ret < 0 ) {
		mConnected = false;
	}
	return ret;
}


int RawWifi::Write( const void* buf, uint32_t len, int timeout )
{
	if ( !mConnected or mOutputPort < 0 ) {
		return -1;
	}

	int ret = rawwifi_send( mRaWifi, (uint8_t*)buf, len );

	if ( ret < 0 ) {
		mConnected = false;
	}
	return ret;
}

#endif // ( BUILD_RAWWIFI == 1 )
