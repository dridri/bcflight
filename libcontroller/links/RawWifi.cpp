#include "RawWifi.h"
#include "../librawwifi++/RawWifi.h"
#include "../librawwifi++/WifiInterfaceLinux.h"
#include <Debug.h>

RawWifi::RawWifi( const std::string& device, int16_t out_port, int16_t in_port, int read_timeout_ms )
	: Link()
{
	// Debug::setDebugLevel( Debug::Level::Trace );

	try {
		mWifiInterface = new rawwifi::WifiInterfaceLinux( device, 11, 18, 20, 3 );
		// mWifiInterface = new rawwifi::WifiInterfaceLinux( device, 11, 18, 0, 54 );
	} catch ( std::exception& e ) {
		gError() << "Failed to create WifiInterface: " << e.what();
		mWifiInterface = nullptr;
	}
	try {
		mRawWifi = new rawwifi::RawWifi( device, in_port, out_port, true, read_timeout_ms );
	} catch ( std::exception& e ) {
		gError() << "Failed to create RawWifi: " << e.what();
		mRawWifi = nullptr;
	}
}


RawWifi::~RawWifi()
{
}


int RawWifi::Connect()
{
	if ( not mRawWifi ) {
		return -1;
	}
	mConnected = true;
	return 0;
}

int RawWifi::setBlocking( bool blocking )
{
	return 0;
}


void RawWifi::setRetriesCount( int retries )
{
}


int RawWifi::retriesCount() const
{
	return 1;
}


int32_t RawWifi::Channel()
{
	return 0;
}


int32_t RawWifi::Frequency()
{
	return 0;
}


int32_t RawWifi::RxQuality()
{
	return 100;
}


int32_t RawWifi::RxLevel()
{
	return -1;
}


int RawWifi::Read( void* buf, uint32_t len, int32_t timeout )
{
	bool valid = false;
	return mRawWifi->Receive( reinterpret_cast<uint8_t*>(buf), len, &valid, timeout );
}


int RawWifi::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
	return mRawWifi->Send( reinterpret_cast<const uint8_t*>(buf), len, 1 );
}


uint32_t RawWifi::fullReadSpeed()
{
	return 0;
}