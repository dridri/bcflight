#pragma once

#include "Link.h"

namespace rawwifi {
	class RawWifi;
	class WifiInterface;
};

class RawWifi : public Link 
{
public:
	RawWifi( const std::string& device, int16_t out_port, int16_t in_port = -1, int read_timeout_ms = -1 );
	virtual ~RawWifi();

	virtual int Connect();
	virtual int setBlocking( bool blocking );
	virtual void setRetriesCount( int retries );
	virtual int retriesCount() const;
	virtual int32_t Channel();
	virtual int32_t Frequency();
	virtual int32_t RxQuality();
	virtual int32_t RxLevel();

	virtual int Read( void* buf, uint32_t len, int32_t timeout );
	virtual int Write( const void* buf, uint32_t len, bool ack, int32_t timeout );

	virtual uint32_t fullReadSpeed();

protected:
	rawwifi::RawWifi* mRawWifi;
	rawwifi::WifiInterface* mWifiInterface;
};
