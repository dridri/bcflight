#ifndef RAWWIFI_H
#define RAWWIFI_H

#if ( BUILD_RAWWIFI == 1 )

#include "Link.h"
#include <wifibroadcast.h>

class Main;

class RawWifi : public Link
{
public:
	RawWifi( const std::string& device, int16_t out_port, int16_t in_port = -1 );
	~RawWifi();

	void SetChannel( int chan );
	void SetTxPower( int dBm );
	int setBlocking( bool blocking );
	int Connect();
	int Read( void* buf, uint32_t len, int timeout = 0 );
	int ReadFloat( float* f );
	int ReadU32( uint32_t* v );
	int Write( void* buf, uint32_t len, int timeout = 0 );
	int WriteU32( uint32_t v );
	int WriteFloat( float v );
	int WriteString( const std::string& s );
	int WriteString( const char* fmt, ... );

	static int flight_register( Main* main );

protected:
	static Link* Instanciate( Config* config, const std::string& lua_object );

	rwifi_tx_t* mTx;
	rwifi_rx_t* mRx;
	std::string mDevice;
	int mChannel;
	int mTxPower;
	int16_t mOutputPort;
	int16_t mInputPort;
};

#endif // ( BUILD_RAWWIFI == 1 )

#endif // RAWWIFI_H
