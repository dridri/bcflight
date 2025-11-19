#include <Debug.h>
#include "RawWifi.h"
#include "constants.h"
#include "ieee80211_radiotap.h"

using namespace rawwifi;

int32_t RawWifi::Send( const uint8_t* buffer, uint32_t size, uint8_t retries )
{
	fDebug( (void*)buffer, size, (int)retries );
	if ( retries > 16 ) {
		gError() << "Too many retries asked (" << retries << ", max is 16)";
		return -1;
	}

	int32_t sent = 0;
	int32_t remain = size;
	uint16_t packet_id = 0;
	const uint8_t* data = buffer;
	uint32_t datalen = size;
/*
	if ( send_block_flags & RAWWIFI_BLOCK_FLAGS_HAMMING84 ) {
		data = (uint8_t*)malloc( datalen_ * 2 );
		datalen = rawwifi_hamming84_encode( data, data_, datalen_ );
		remain = datalen;
	}
*/
	uint16_t packets_count = ( datalen / ( RAWWIFI_MAX_USER_PACKET_LENGTH - mHeadersLenght ) ) + 1;

	mSendBlockId++;

	while ( sent < (int32_t)datalen ) {
		int32_t len = RAWWIFI_MAX_USER_PACKET_LENGTH - mHeadersLenght;
		if ( len > remain ) {
			len = remain;
		}

		SendFrame( data + sent, len, mSendBlockId, BLOCK_FLAGS_NONE, packet_id, packets_count, retries );

		packet_id++;
		sent += len;
		remain -= len;
	}
/*
	if ( rwifi->send_block_flags & RAWWIFI_BLOCK_FLAGS_HAMMING84 ) {
		free( data );
	}
*/
	return sent;
}


int32_t RawWifi::SendFrame( const uint8_t* data, uint32_t datalen, uint32_t block_id, BlockFlags block_flags, uint16_t packet_id, uint16_t packets_count, uint32_t retries )
{
	mTxMutex.lock();

	mTxBuffer[sizeof(RadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = mTxPort;
	mTxBuffer[sizeof(RadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = mTxPort;

	PacketHeader* header = (PacketHeader*)( mTxBuffer + mHeadersLenght - sizeof( PacketHeader ) );
	header->block_id = block_id;
	header->packet_id = packet_id;
	header->packets_count = packets_count;
	header->retries_count = retries;
	header->block_flags = block_flags;
	header->crc = crc32( data, datalen );

	memcpy( mTxBuffer + mHeadersLenght, data, datalen );
	int32_t plen = datalen + mHeadersLenght;

	int r = 0;
	for ( uint32_t i = 0; i < retries; i++ ) {
		header->retry_id = i;
		header->header_crc = crc16( (uint8_t*)header, sizeof(PacketHeader) - sizeof(PacketHeader::header_crc) );
		r = pcap_inject( mTxPcap->getPcap(), mTxBuffer, plen );
		if ( r != plen ) {
			char* err = pcap_geterr( mTxPcap->getPcap() );
			gError() << "[" << i + 1 << "/" << retries << "] Trouble injecting packet : " << err << ". Sent " << r << " / " << plen;
			mTxMutex.unlock();
			return -1;
		} else {
			gTrace() << "[" << block_id << ", " << packet_id << ", " << i + 1 << "/" << retries << "] Sent " << r << " / " << plen;
		}
	}

	mTxMutex.unlock();
	return plen;
}


void RawWifi::initTxBuffer( uint8_t* buffer, uint32_t size )
{
	RadiotapHeader radiotapHeader;
	memset( &radiotapHeader, 0, sizeof(RadiotapHeader) );

	radiotapHeader.version = 0;
	radiotapHeader.padding = 0;
	radiotapHeader.length = sizeof(RadiotapHeader);
	radiotapHeader.presence = /*(1 << IEEE80211_RADIOTAP_RATE) |*/ (1 << IEEE80211_RADIOTAP_TX_FLAGS);
	// radiotapHeader.rate = 54000000 / 500000; // 54mbps
	radiotapHeader.txFlags = /*IEEE80211_RADIOTAP_F_FCS | */IEEE80211_RADIOTAP_F_TX_NOACK;

	if ( true ) {
		radiotapHeader.presence |= (1 << IEEE80211_RADIOTAP_MCS);
		radiotapHeader.mcs.known = IEEE80211_RADIOTAP_MCS_HAVE_MCS
			| IEEE80211_RADIOTAP_MCS_HAVE_BW
			| IEEE80211_RADIOTAP_MCS_HAVE_GI
			| IEEE80211_RADIOTAP_MCS_HAVE_STBC
			| IEEE80211_RADIOTAP_MCS_HAVE_FEC
			;
		radiotapHeader.mcs.modulationIndex = 3;
		radiotapHeader.mcs.flags = IEEE80211_RADIOTAP_MCS_BW_20;
		// radiotapHeader.mcs.flags |= IEEE80211_RADIOTAP_MCS_SGI;
		radiotapHeader.mcs.flags |= IEEE80211_RADIOTAP_MCS_FEC_LDPC;
	}

	/* Penumbra IEEE80211 header */
	static uint8_t u8aIeeeHeader[] = {
		0x08, 0x01, // Control fields
		0x00, 0x00, // Duration
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
		0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
		// 0x10, 0x86,
		0x00, 0x00,
	};

	memcpy( buffer, &radiotapHeader, sizeof( radiotapHeader ) );
	buffer += sizeof( radiotapHeader );
	memcpy( buffer, u8aIeeeHeader, sizeof( u8aIeeeHeader ) );
	buffer += sizeof( u8aIeeeHeader );

	mHeadersLenght = sizeof( radiotapHeader ) + sizeof( u8aIeeeHeader ) + sizeof(PacketHeader);
}
