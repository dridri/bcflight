#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "rawwifi.h"

static const uint8_t u8aRadiotapHeader[] = {
	0x00, 0x00, // <-- radiotap version, pad
	0x0c, 0x00, // <- radiotap header length
	0x04, 0x80, 0x00, 0x00, // IEEE80211_RADIOTAP_RATE, IEEE80211_RADIOTAP_TX_FLAGS
// 	0x6c, // 54mbps
	0x22,
	0x0, // pad
	0x10 | 0x08, 0x00, // IEEE80211_RADIOTAP_F_FCS | IEEE80211_RADIOTAP_F_TX_NOACK
};

/* Penumbra IEEE80211 header */
static uint8_t u8aIeeeHeader[] = {
	0x08, 0x01, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
// 	0xb8, 0x27, 0xeb, 0xc7, 0x6b, 0x75,
// 	0xb8, 0x27, 0xeb, 0xc7, 0x6b, 0x75,
	0x10, 0x86,
};

uint8_t* rawwifi_radiotap_header() { return u8aRadiotapHeader; }

// #define DEBUG
#ifdef DEBUG
#define dprintf printf
#else
#define dprintf(...) ;
#endif

static pthread_mutex_t inject_mutex = PTHREAD_MUTEX_INITIALIZER; 
const uint32_t rawwifi_headers_length = sizeof( u8aRadiotapHeader ) + sizeof( u8aIeeeHeader ) + sizeof( wifi_packet_header_t );


void rawwifi_init_txbuf( uint8_t* buf )
{

	memcpy( buf, u8aRadiotapHeader, sizeof( u8aRadiotapHeader ) );
	buf += sizeof( u8aRadiotapHeader );
	memcpy( buf, u8aIeeeHeader, sizeof( u8aIeeeHeader ) );
	buf += sizeof( u8aIeeeHeader );
}


static int rawwifi_send_frame( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t block_id, RAWWIFI_BLOCK_FLAGS block_flags, uint16_t packet_id, uint16_t packets_count, uint32_t retries )
{
#ifdef DEBUG
	uint32_t i = 0;
#endif
	uint8_t* tx_buffer = rwifi->tx_buffer;
	if ( rwifi->send_block_flags & RAWWIFI_BLOCK_FLAGS_EXTRA_HEADER_ROOM ) {
		tx_buffer = data - rawwifi_headers_length;
		rawwifi_init_txbuf( tx_buffer );
	}

	tx_buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->out->port;
	tx_buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->out->port;

	wifi_packet_header_t* header = (wifi_packet_header_t*)( tx_buffer + sizeof( u8aRadiotapHeader ) + sizeof( u8aIeeeHeader ) );
	header->block_id = block_id;
	header->packet_id = packet_id;
	header->packets_count = packets_count;
	header->retries_count = retries;
	header->block_flags = block_flags;
	header->crc = rawwifi_crc32( data, datalen );

	if ( ! ( rwifi->send_block_flags & RAWWIFI_BLOCK_FLAGS_EXTRA_HEADER_ROOM ) ) {
		memcpy( tx_buffer + rawwifi_headers_length, data, datalen );
	}
	int plen = datalen + rawwifi_headers_length;

	int r = 0;
	for ( uint32_t i = 0; i < retries; i++ ) {
		header->retry_id = i;
		header->header_crc = rawwifi_crc16( (uint8_t*)header, sizeof(wifi_packet_header_t) - sizeof(uint16_t) );
// retry:
// 		pthread_mutex_lock( &inject_mutex );
		r = pcap_inject( rwifi->out->pcap, tx_buffer, plen );
// 		pthread_mutex_unlock( &inject_mutex );
		if ( r != plen ) {
			pcap_perror( rwifi->out->pcap, "Trouble injecting packet" );
			printf( "[%d/%d] sent %d / %d\n", i + 1, retries, r, plen );
// 			exit(1);
// 			usleep( 1 );
// 			goto retry;
			return -1;
		} else {
			dprintf( "[%d, %d, %d/%d] sent %d / %d\n", block_id, packet_id, i + 1, retries, r, plen );
		}
	}

	return plen;
}


int rawwifi_send_retry( rawwifi_t* rwifi, uint8_t* data_, uint32_t datalen_, uint32_t retries )
{
	int sent = 0;
	int remain = datalen_;
	int len = 0;
	uint16_t packet_id = 0;
	uint8_t* data = data_;
	uint32_t datalen = datalen_;

	if ( rwifi->send_block_flags & RAWWIFI_BLOCK_FLAGS_HAMMING84 ) {
		data = (uint8_t*)malloc( datalen_ * 2 );
		datalen = rawwifi_hamming84_encode( data, data_, datalen_ );
		remain = datalen;
	}

	uint16_t packets_count = ( datalen / ( MAX_USER_PACKET_LENGTH - rawwifi_headers_length ) ) + 1;

	if ( rwifi->max_block_size > 0 && retries * datalen > rwifi->max_block_size ) {
		retries = ( rwifi->max_block_size / datalen );
	}

	rwifi->send_block_id++;

	while ( sent < datalen ) {
		len = MAX_USER_PACKET_LENGTH - rawwifi_headers_length;
		if ( len > remain ) {
			len = remain;
		}

		rawwifi_send_frame( rwifi, data + sent, len, rwifi->send_block_id, rwifi->send_block_flags, packet_id, packets_count, retries );

		packet_id++;
		sent += len;
		remain -= len;
	}

	if ( rwifi->send_block_flags & RAWWIFI_BLOCK_FLAGS_HAMMING84 ) {
		free( data );
	}
	return sent;
}


int rawwifi_send( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen )
{
	return rawwifi_send_retry( rwifi, data, datalen, 1 );
}
