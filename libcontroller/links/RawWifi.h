#ifndef RAWWIFI_H
#define RAWWIFI_H

#ifndef NO_RAWWIFI

#include "Link.h"

#include <rawwifi.h>

class RawWifi : public Link
{
public:
	RawWifi( const std::string& device, int16_t out_port, int16_t in_port = -1 );
	~RawWifi();

	void SetChannel( int chan );
	void SetTxPower( int dBm );
	int setBlocking( bool blocking );
	int Connect();
	int32_t RxQuality();

protected:
	static void Initialize( const std::string& device, uint32_t channel, uint32_t txpower );
	int Read( void* buf, uint32_t len, int32_t timeout );
	int Write( const void* buf, uint32_t len, int32_t timeout );

	rawwifi_t* mRawWifi;
	std::string mDevice;
	int mChannel;
	int mTxPower;
	int16_t mOutputPort;
	int16_t mInputPort;

	static bool mInitialized;
};

#endif // NO_RAWWIFI

#endif // RAWWIFI_H
