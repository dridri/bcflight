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


static int rawwifi_send_frame( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t block_id, uint16_t packet_id, uint16_t packets_count, uint32_t retries )
{
#ifdef DEBUG
	uint32_t i = 0;
#endif
	uint8_t buffer[MAX_USER_PACKET_LENGTH];
	uint8_t* pu8 = buffer;

	memcpy( pu8, u8aRadiotapHeader, sizeof( u8aRadiotapHeader ) );
	pu8 += sizeof( u8aRadiotapHeader );
	memcpy( pu8, u8aIeeeHeader, sizeof( u8aIeeeHeader ) );
	pu8 += sizeof( u8aIeeeHeader );

	buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->out->port;
	buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->out->port;

	wifi_packet_header_t header = {
		.block_id = block_id,
		.packet_id = packet_id,
		.packets_count = packets_count,
		.crc = rawwifi_crc32( data, datalen )
	};
	if ( retries & RETRY_ACK ) {
		header.packets_count = packets_count | RETRY_ACK;
	}
	memcpy( pu8, &header, sizeof( header ) );
	pu8 += sizeof( header );

	memcpy( pu8, data, datalen );
	int plen = datalen + headers_length;

	if ( retries & RETRY_ACK ) {
		int ok = 0;
		while ( ok == 0 ) {
			int r = pcap_inject( rwifi->out->pcap, buffer, plen );
			if ( r != plen ) {
				pcap_perror( rwifi->out->pcap, "Trouble injecting packet" );
				dprintf( "[%d, %d, %d] sent %d / %d\n", block_id, packet_id, i++, r, plen );
// 				exit(1);
				continue;
			}
			dprintf( "[%d] sent %d bytes\n", i++, r );
			wifi_packet_ack_t ack;
			while ( 1 ) {
				int32_t ret = rawwifi_recv_ack( rwifi, &ack );
				if ( ret <= 0 ) {
					dprintf( "ACK Error\n" );
					break;
				}
				if ( ret == sizeof( ack ) && ack.block_id == block_id && ack.packet_id == packet_id && ack.valid != 0 ) {
					dprintf( "ACK Ok\n" );
					ok = 1;
					break;
				}
				if ( ret == sizeof( ack ) && ( ack.block_id < block_id || ( ack.block_id == block_id && ack.packet_id < packet_id ) ) ) {
					dprintf( "ACK too old\n" );
					continue;
				}
			}
		}
	} else {
		int r = 0;
		for ( uint32_t i = 0; i < retries; i++ ) {
retry:
			r = pcap_inject( rwifi->out->pcap, buffer, plen );
			if ( r != plen ) {
				pcap_perror( rwifi->out->pcap, "Trouble injecting packet" );
				printf( "[%d/%d] sent %d / %d\n", i + 1, retries, r, plen );
// 				exit(1);
				usleep( 100 );
				goto retry;
			} else {
				dprintf( "[%d, %d, %d/%d] sent %d / %d\n", block_id, packet_id, i + 1, retries, r, plen );
			}
		}
	}

	return plen;
}


int _rawwifi_send_retry( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t retries )
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


int rawwifi_send_retry( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t retries )
{
	if ( rwifi->out->blocking == 0 ) {
		pthread_mutex_lock( &rwifi->send_mutex );
		rwifi->send_queue = (uint8_t*)malloc( datalen );
		rwifi->send_queue_size = datalen;
		rwifi->send_queue_retries = retries;
		memcpy( rwifi->send_queue, data, datalen );
		pthread_mutex_unlock( &rwifi->send_mutex );
		pthread_cond_signal( &rwifi->send_cond );
		return datalen;
	}
	return _rawwifi_send_retry( rwifi, data, datalen, retries );
}


int rawwifi_send( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen )
{
	return rawwifi_send_retry( rwifi, data, datalen, 1 );
}


int32_t rawwifi_send_ack( rawwifi_t* rwifi, wifi_packet_ack_t* ack )
{
	uint8_t buffer[MAX_USER_PACKET_LENGTH];
	uint8_t* pu8 = buffer;

	memcpy( pu8, u8aRadiotapHeader, sizeof( u8aRadiotapHeader ) );
	pu8 += sizeof( u8aRadiotapHeader );
	memcpy( pu8, u8aIeeeHeader, sizeof( u8aIeeeHeader ) );
	pu8 += sizeof( u8aIeeeHeader );

	buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->in_ack->port;
	buffer[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = rwifi->in_ack->port;

	memcpy( pu8, ack, sizeof( wifi_packet_ack_t ) );
	pu8 += sizeof( wifi_packet_ack_t );

	const int RETRIES = 1;
	int plen = sizeof( u8aRadiotapHeader ) + sizeof( u8aIeeeHeader ) + sizeof( wifi_packet_ack_t );
	for ( uint32_t i = 0; i < RETRIES; i++ ) {
		int r = 0;
retry:
		r = pcap_inject( rwifi->in_ack->pcap, buffer, plen );
		if ( r != plen ) {
			pcap_perror( rwifi->in_ack->pcap, "Trouble injecting ACK packet" );
			dprintf( "[%d/%d] ACK sent %d / %d\n", i + 1, RETRIES, r, plen );
// 			exit(1);
			goto retry;
		} else {
			dprintf( "[%d/%d] ACK sent %d bytes on port %d\n", i + 1, RETRIES, r, rwifi->in_ack->port );
		}
	}

	return sizeof( wifi_packet_ack_t );
}