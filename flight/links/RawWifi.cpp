#if ( BUILD_RAWWIFI == 1 )

#include <netinet/in.h>
#include <string.h>
#include <Debug.h>
#include "RawWifi.h"
#include "../Config.h"
#include <Board.h>

bool RawWifi::mInitialized = false;


int RawWifi::flight_register( Main* main )
{
	RegisterLink( "RawWifi", &RawWifi::Instanciate );
	return 0;
}


Link* RawWifi::Instanciate( Config* config, const std::string& lua_object )
{
	std::string device = config->string( lua_object + ".device" );
	int output_port = config->integer( lua_object + ".output_port" );
	int input_port = config->integer( lua_object + ".input_port" );
	bool blocking = config->boolean( lua_object + ".blocking" );

	Link* link = new RawWifi( device, output_port, input_port, blocking );
	static_cast< RawWifi* >( link )->setRetries( config->integer( lua_object + ".retries" ) );
	return link;
}



RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port, bool blocking )
	: Link()
	, mRawWifi( nullptr )
	, mDevice( device )
	, mChannel( 9 )
	, mTxPower( 33 )
	, mOutputPort( out_port )
	, mInputPort( in_port )
	, mBlocking( blocking )
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


void RawWifi::Initialize( const std::string& device, uint32_t channel, uint32_t txpower )
{
	if ( not mInitialized ) {
		mInitialized = true;
		std::stringstream ss;

		if ( Board::readcmd( "ifconfig " + device + " | grep " + device, "encap", ":" ).find( "UNSPEC" ) == std::string::npos ) {
			ss << "ifconfig " << device << " down";
			ss << " && iw dev " << device << " set monitor otherbss fcsfail";
			ss << " && ifconfig " << device << " up && ";
		}
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
	Initialize( mDevice, mChannel, mTxPower );

	mRawWifi = rawwifi_init( "wlan0", mOutputPort, mInputPort, mBlocking );
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
	if ( not valid ) {
// 		gDebug() << "WARNING : Received corrupt packets\n";
		// TBD : keep it or return 0 ?
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
