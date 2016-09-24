#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "rawwifi.h"

static const uint8_t u8aRadiotapHeader[] = {

	0x00, 0x00, // <-- radiotap version
	0x0c, 0x00, // <- radiotap header length
	0x04, 0x80, 0x00, 0x00, // <-- bitmap
	0x22, 
	0x0, 
	0x18, 0x00 
};

/* Penumbra IEEE80211 header */
static uint8_t u8aIeeeHeader[] = {
	0x08, 0x01, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
	0x10, 0x86,
};

// #define DEBUG
#ifdef DEBUG
#define dprintf printf
#else
#define dprintf(...) ;
#endif

static const uint32_t headers_length = sizeof( u8aRadiotapHeader ) + sizeof( u8aIeeeHeader ) + sizeof( wifi_packet_header_t );


void rawwifi_init_txbuf( uint8_t* buf )
{

	memcpy( buf, u8aRadiotapHeader, sizeof( u8aRadiotapHeader ) );
	buf += sizeof( u8aRadiotapHeader );
	memcpy( buf, u8aIeeeHeader, sizeof( u8aIeeeHeader ) );
	buf += sizeof( u8aIeeeHeader );
}


static int rawwifi_send_frame( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t block_id, uint16_t packet_id, uint16_t packets_count, uint32_t retries )
{
#ifdef DEBUG
	uint32_t i = 0;
#endif
	uint8_t* tx_buffer = rwifi->tx_buffer;

	tx_buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->out->port;
	tx_buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->out->port;

	wifi_packet_header_t* header = (wifi_packet_header_t*)( tx_buffer + sizeof( u8aRadiotapHeader ) + sizeof( u8aIeeeHeader ) );
	header->block_id = block_id;
	header->packet_id = packet_id;
	header->packets_count = packets_count;
	header->crc = rawwifi_crc32( data, datalen );

	memcpy( tx_buffer + headers_length, data, datalen );
	int plen = datalen + headers_length;

	int r = 0;
	for ( uint32_t i = 0; i < retries; i++ ) {
retry:
		r = pcap_inject( rwifi->out->pcap, tx_buffer, plen );
		if ( r != plen ) {
			pcap_perror( rwifi->out->pcap, "Trouble injecting packet" );
			printf( "[%d/%d] sent %d / %d\n", i + 1, retries, r, plen );
// 			exit(1);
			usleep( 1 );
			goto retry;
		} else {
			dprintf( "[%d, %d, %d/%d] sent %d / %d\n", block_id, packet_id, i + 1, retries, r, plen );
		}
	}

	return plen;
}


int rawwifi_send_retry( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t retries )
{
	int sent = 0;
	int remain = datalen;
	int len = 0;
	uint16_t packet_id = 0;
	uint16_t packets_count = ( datalen / ( MAX_USER_PACKET_LENGTH - headers_length ) ) + 1;

	rwifi->send_block_id++;

	while ( sent < datalen ) {
		len = MAX_USER_PACKET_LENGTH - headers_length;
		if ( len > remain ) {
			len = remain;
		}

		rawwifi_send_frame( rwifi, data + sent, len, rwifi->send_block_id, packet_id, packets_count, retries );

		packet_id++;
		sent += len;
		remain -= len;
	}

	return sent;
}


int rawwifi_send( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen )
{
	return rawwifi_send_retry( rwifi, data, datalen, 1 );
}
