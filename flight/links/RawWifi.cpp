#if ( BUILD_RAWWIFI == 1 )

#include <netinet/in.h>
#include <string.h>
#include <Debug.h>
#include "RawWifi.h"
#include "../Config.h"


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

	return new RawWifi( device, output_port, input_port );
}



RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port )
	: Link()
	, mTx( nullptr )
	, mRx( nullptr )
	, mDevice( device )
	, mChannel( 13 )
	, mTxPower( 0 )
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

	if ( mOutputPort >= 0 ) {
		mTx = rwifi_tx_init( mDevice.c_str(), mOutputPort, 1 );
		if ( !mTx ) {
			return -1;
		}
	}

	if ( mInputPort >= 0 ) {
		mRx = rwifi_rx_init( mDevice.c_str(), mInputPort, 1 );
		if ( !mRx ) {
			return -1;
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

	int ret = rwifi_rx_recv( mRx, (uint8_t*)buf, len );

	if ( ret < 0 ) {
		mConnected = false;
	}
	return ret;
}


int RawWifi::ReadFloat( float* f )
{
	union {
		float f;
		uint32_t u;
	} d;
	int ret = Read( &d.u, sizeof( uint32_t ) );
	d.u = ntohl( d.u );
	*f = d.f;
	return ret;
}


int RawWifi::ReadU32( uint32_t* v )
{
	int ret = Read( v, sizeof( uint32_t ) );
	if ( ret == sizeof( uint32_t ) ) {
		*v = ntohl( *v );
	}
	return ret;
}


int RawWifi::Write( void* buf, uint32_t len, int timeout )
{
	if ( !mConnected or mOutputPort < 0 ) {
		return -1;
	}

	int ret = rwifi_tx_send( mTx, (uint8_t*)buf, len );

	if ( ret < 0 ) {
		mConnected = false;
	}
	return ret;
}


int RawWifi::WriteU32( uint32_t v )
{
	v = htonl( v );
	return Write( &v, sizeof(v) );
}


int RawWifi::WriteFloat( float v )
{
	union {
		float f;
		uint32_t u;
	} u;
	u.f = v;
	u.u = htonl( u.u );
	return Write( &u, sizeof( u ) );
}


int RawWifi::WriteString( const std::string& s )
{
	return Write( (void*)s.c_str(), s.length() );
}


int RawWifi::WriteString( const char* fmt, ... )
{
	va_list opt;
	char buff[1024] = "";
	va_start( opt, fmt );
	vsnprintf( buff, (size_t) sizeof(buff), fmt, opt );
	return Write( buff, strlen(buff) + 1 );
}

#endif // ( BUILD_RAWWIFI == 1 )
