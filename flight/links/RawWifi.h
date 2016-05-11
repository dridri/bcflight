#ifndef RAWWIFI_H
#define RAWWIFI_H

#include "Link.h"

#if ( BUILD_RAWWIFI == 1 )

#include <rawwifi.h>

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

	static int flight_register( Main* main );

protected:
	static Link* Instanciate( Config* config, const std::string& lua_object );
	int Read( void* buf, uint32_t len, int32_t timeout );
	int Write( const void* buf, uint32_t len, int32_t timeout );

	rawwifi_t mRaWifi;
	std::string mDevice;
	int mChannel;
	int mTxPower;
	int16_t mOutputPort;
	int16_t mInputPort;
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
