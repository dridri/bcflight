#include <Debug.h>
#include "RawWifi.h"
#include "constants.h"

using namespace rawwifi;

RawWifi::RawWifi( const std::string& device, uint8_t rx_port, uint8_t tx_port, bool blocking, int32_t read_timeout_ms )
	: mDevice( device )
	, mRxPort( rx_port )
	, mTxPort( tx_port )
	, mBlocking( blocking )
	, mReadTimeoutMs( read_timeout_ms )
	, mIwSocket( -1 )
	, mRxPcap( nullptr )
	, mRxFecMode( RxFecMode::RX_FAST )
	, mRxBlockRecoverMode( RxBlockRecoverMode::FILL_WITH_ZEROS )
	, mRxBlock({ 0xFFFFFFFF, 0, 0, 0 })
	, mRxLastCompletedBlockId( 0 )
	, mTxPcap( nullptr )
	, mSendBlockId( 0 )
{
	fDebug( device, (int)rx_port, (int)tx_port, (int)blocking, read_timeout_ms );

	mCompileMutex.lock();

	try {
		mRxPcap = new PcapHandler( device, rx_port, blocking, read_timeout_ms );
		mRxPcap->CompileFilter();
	} catch ( std::exception& e ) {
		gError() << "Failed to setup RawWifi RX : " << e.what();
	}

	try {
		mTxPcap = new PcapHandler( device, tx_port, false, 1 );
	} catch ( std::exception& e ) {
		gError() << "Failed to setup RawWifi TX : " << e.what();
	}

	if ( !mRxPcap or !mTxPcap ) {
		mCompileMutex.unlock();
		throw std::runtime_error( "Failed to setup RX/TX" );
	}

	initTxBuffer( mTxBuffer, sizeof( mTxBuffer ) );
	mCompileMutex.unlock();
}


RawWifi::~RawWifi()
{
	if ( mRxPcap ) {
		delete mRxPcap;
	}
	if ( mTxPcap ) {
		delete mTxPcap;
	}
}


uint32_t RawWifi::crc32( const uint8_t* buf, uint32_t len )
{
	uint32_t k = 0;
	uint32_t crc = 0;

	crc = ~crc;

	while ( len-- ) {
		crc ^= *buf++;
		for ( k = 0; k < 8; k++ ) {
			crc = ( crc & 1 ) ? ( (crc >> 1) ^ 0x82f63b78 ) : ( crc >> 1 );
		}
	}

	return ~crc;
}


uint16_t RawWifi::crc16( const uint8_t* buf, uint32_t len )
{
	uint8_t x;
	uint16_t crc = 0xFFFF;

	while ( len-- ) {
		x = crc >> 8 ^ *buf++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
	}

	return crc;
}
