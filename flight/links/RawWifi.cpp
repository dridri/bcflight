#include "RawWifi.h"

#if (BUILD_RAWWIFI == 1)

#include "../librawwifi++/RawWifi.h"
#include "../librawwifi++/WifiInterfaceLinux.h"
#include <Debug.h>


RawWifi::RawWifi()
	: Link()
	, Thread( "RawWifi" )
	, mDevice( "wlan0" )
	, mOutputPort( 0 )
	, mInputPort( 1 )
	, mChannel( 9 )
	, mBlocking( true )
	, mRawWifi( nullptr )
	, mWifiInterface( nullptr )
{
}


RawWifi::~RawWifi()
{
	mConnected = false;
	if ( mRawWifi ) {
		delete mRawWifi;
	}
	if ( mWifiInterface ) {
		delete mWifiInterface;
	}
}

int RawWifi::Connect()
{
	// Debug::setDebugLevel( Debug::Level::Trace );

	try {
		mWifiInterface = new rawwifi::WifiInterfaceLinux( mDevice, 11, 18, 20, 3 );
		// mWifiInterface = new rawwifi::WifiInterfaceLinux( mDevice, 11, 18, 0, 54 );
	} catch ( std::exception& e ) {
		gError() << "Failed to create WifiInterface: " << e.what();
		mWifiInterface = nullptr;
		return -1;
	}

	try {
		mRawWifi = new rawwifi::RawWifi( mDevice, mInputPort, mOutputPort, mBlocking, 0 );
	} catch ( std::exception& e ) {
		gError() << "Failed to create RawWifi: " << e.what();
		mRawWifi = nullptr;
		return -1;
	}

	Thread::setName( "RawWifi[" + mDevice + "][" + to_string( mInputPort ) + "," + to_string( mOutputPort ) + "]" );
	Thread::Start();

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
	return mChannel;
}


int32_t RawWifi::Frequency()
{
	return 0;
}


int32_t RawWifi::RxQuality()
{
	return 0;
}


int32_t RawWifi::RxLevel()
{
	return 0;
}


SyncReturn RawWifi::Read( void* buf, uint32_t len, int32_t timeout )
{
	fDebug( buf, len, timeout );
	bool valid = false;
	return mRawWifi->Receive( reinterpret_cast<uint8_t*>(buf), len, &valid, timeout );
}


SyncReturn RawWifi::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
	// return mRawWifi->Send( reinterpret_cast<const uint8_t*>(buf), len, 1 );
	std::lock_guard<std::mutex> lck(mPacketsQueueMutex);
	mPacketsQueue.push_back( std::vector<uint8_t>( reinterpret_cast<const uint8_t*>(buf), reinterpret_cast<const uint8_t*>(buf) + len ) );
	mPacketsQueueCond.notify_one();
	return len;
}


bool RawWifi::run()
{
    std::unique_lock<std::mutex> lk(mPacketsQueueMutex);
	mPacketsQueueCond.wait( lk, [this]{ return not mPacketsQueue.empty(); } );
	lk.unlock();

	mPacketsQueueMutex.lock();
	while ( not mPacketsQueue.empty() ) {
		std::vector<uint8_t> packet = mPacketsQueue.front();
		mPacketsQueue.pop_front();
		mPacketsQueueMutex.unlock();
		mRawWifi->Send( packet.data(), packet.size(), 1 );
		mPacketsQueueMutex.lock();
	}
	mPacketsQueueMutex.unlock();

	return true;
}

#else // BUILD_RAWWIFI


RawWifi::RawWifi()
	: Link()
	, Thread( "RawWifi" )
	, mDevice( "wlan0" )
	, mOutputPort( 0 )
	, mInputPort( 1 )
	, mChannel( 9 )
	, mBlocking( true )
	, mRawWifi( nullptr )
	, mWifiInterface( nullptr )
{
}


RawWifi::~RawWifi()
{
}

int RawWifi::Connect()
{
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
	return mChannel;
}


int32_t RawWifi::Frequency()
{
	return 0;
}


int32_t RawWifi::RxQuality()
{
	return 0;
}


int32_t RawWifi::RxLevel()
{
	return 0;
}


SyncReturn RawWifi::Read( void* buf, uint32_t len, int32_t timeout )
{
	return 0;
}


SyncReturn RawWifi::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
	return 0;
}


bool RawWifi::run()
{
	return false;
}

#endif // BUILD_RAWWIFI
