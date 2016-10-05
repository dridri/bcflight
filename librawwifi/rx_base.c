#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <iwlib.h>

#include "rawwifi.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef u32 __le32;

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define	le16_to_cpu(x) (x)
#define	le32_to_cpu(x) (x)
#else
#define	le16_to_cpu(x) ((((x)&0xff)<<8)|(((x)&0xff00)>>8))
#define	le32_to_cpu(x) \
((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)&0xff0000)>>8)|(((x)&0xff000000)>>24))
#endif
#define	unlikely(x) (x)

#define	MAX_PENUMBRA_INTERFACES 8

#include "radiotap.h"

#define CONTINUE -2

// #define DEBUG
#ifdef DEBUG
#define dprintf(...) fprintf( stderr, __VA_ARGS__ )
#else
#define dprintf(...) ;
#endif

typedef struct  {
	int m_nChannel;
	int m_nChannelFlags;
	int m_nRate;
	int m_nAntenna;
	int m_nRadiotapFlags;
} __attribute__((packed)) PENUMBRA_RADIOTAP_DATA;

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

const uint32_t _rawwifi_headers_length = sizeof( u8aRadiotapHeader ) + sizeof( u8aIeeeHeader ) + sizeof( wifi_packet_header_t );
uint64_t _rawwifi_get_tick();
int process_packet_weighted( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* pret, uint32_t retmax, uint32_t* valid );

int process_frame( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* payloadBuffer, uint8_t** ppu8Payload )
{
	struct pcap_pkthdr* ppcapPacketHeader = NULL;
	struct ieee80211_radiotap_iterator rti;
	PENUMBRA_RADIOTAP_DATA prd = { 0, 0, 0, 0, 0 };
	int bytes;
	int retval;
	int u16HeaderLen;

	uint8_t* pu8Payload = payloadBuffer;

	uint64_t read_ticks = _rawwifi_get_tick();
	retval = pcap_next_ex( rpcap->pcap, &ppcapPacketHeader, (const uint8_t**)&pu8Payload );

	if ( rwifi->recv_timeout_ms > 0 && retval == 0 && _rawwifi_get_tick() - read_ticks >= rwifi->recv_timeout_ms * 1000 ) {
		return -3;
	}

	if ( retval < 0 || pu8Payload == 0 ) {
		fprintf( stderr, "pcap_next_ex ERROR : %s (%d - %s)\n", pcap_geterr( rpcap->pcap ), errno, strerror(errno) );
// 		char str[1024] = "";
// 		sprintf( str, "ifconfig %s up", rwifi->device );
// 		exit(1);
// 		return CONTINUE;
		return -1;
	}

	if ( retval != 1 ) {
		if ( rpcap->blocking == 1 ) {
			dprintf( "retval = %d !!\n", retval );
		}
		return 0;
	}

	u16HeaderLen = (pu8Payload[2] + (pu8Payload[3] << 8));

	if ( ppcapPacketHeader->len < ( u16HeaderLen + rwifi->n80211HeaderLength ) ) {
		dprintf( "ppcapPacketHeader->len < ( u16HeaderLen + rwifi->n80211HeaderLength )\n" );
		return 0;
	}

	bytes = ppcapPacketHeader->len - ( u16HeaderLen + rwifi->n80211HeaderLength );
	if ( bytes < 0 ) {
		dprintf( "bytes < 0\n" );
		return 0;
	}

	if ( ieee80211_radiotap_iterator_init( &rti, (struct ieee80211_radiotap_header *)pu8Payload, ppcapPacketHeader->len ) < 0 ) {
		dprintf( "ieee80211_radiotap_iterator_init( &rti, (struct ieee80211_radiotap_header *)pu8Payload, ppcapPacketHeader->len ) < 0\n" );
		return 0;
	}

// 	int link_update = 0;
	int n = 0;
	while ( ( n = ieee80211_radiotap_iterator_next(&rti) ) == 0 ) {
		switch ( rti.this_arg_index ) {
			case IEEE80211_RADIOTAP_RATE:
				rwifi->recv_link.rate = (*rti.this_arg);
// 				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_CHANNEL:
// 				rwifi->recv_link.channel = le16_to_cpu(*((u16 *)rti.this_arg));
				prd.m_nChannelFlags = le16_to_cpu(*((u16 *)(rti.this_arg + 2)));
// 				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_ANTENNA:
				rwifi->recv_link.antenna = (*rti.this_arg) + 1;
// 				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_FLAGS:
				prd.m_nRadiotapFlags = *rti.this_arg;
// 				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
				rwifi->recv_link.signal = (int8_t)(*rti.this_arg);
// 				link_update = 1;
// 				printf( "RSSI : %d\n", (int8_t)(*rti.this_arg) );
				break;

			default:
				break;
		}
	}

	pu8Payload += u16HeaderLen + rwifi->n80211HeaderLength;

#ifndef __arm__
// 	bytes -= 4;
	if ( prd.m_nRadiotapFlags & IEEE80211_RADIOTAP_F_FCS ) {
		bytes -= 4;
	}
#endif

	int checksum_correct = ( prd.m_nRadiotapFlags & 0x40 ) == 0;
	if ( !checksum_correct ) {
		// TODO/TBD
	}

	*ppu8Payload = pu8Payload;
	return bytes;
}


int analyze_packet( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, wifi_packet_header_t** pHeader, uint8_t** pPayload, uint32_t* valid )
{
	uint8_t* pu8Payload = 0;

	int32_t bytes = process_frame( rwifi, rpcap, pu8Payload, &pu8Payload );
	if ( pu8Payload == 0 || bytes <= 0 ) {
		return bytes;
	}

	wifi_packet_header_t* header = (wifi_packet_header_t*)pu8Payload;
	pu8Payload += sizeof( wifi_packet_header_t );
	bytes -= sizeof( wifi_packet_header_t );

	if ( bytes <= 0 || bytes > MAX_USER_PACKET_LENGTH - sizeof( wifi_packet_header_t ) || header->packet_id >= header->packets_count || header->packet_id > MAX_PACKET_PER_BLOCK || header->packets_count > MAX_PACKET_PER_BLOCK ) {
		*valid = 0;
		return CONTINUE;
	}
	if ( rwifi->recv_last_returned >= header->block_id + 32 ) {
		// More than 32 underruns, TX has probably been resetted
		rwifi->recv_last_returned = 0;
		free( rwifi->recv_block );
		rwifi->recv_block = NULL;
	}

	if ( header->header_crc != rawwifi_crc16( (uint8_t*)header, sizeof(wifi_packet_header_t) - sizeof(uint16_t) ) ) {
		dprintf( "Invalid header CRC, dropping ! [%d:%d]\n", header->block_id, header->packet_id );
		return CONTINUE;
	}

	uint32_t calculated_crc = rawwifi_crc32( pu8Payload, bytes );
	int is_valid = ( header->crc == calculated_crc );
	if ( is_valid == 0 ) {
		dprintf( "Invalid CRC ! (%08X != %08X)\n", header->crc, calculated_crc );
	}

	if ( header->block_id <= rwifi->recv_last_returned ) {
// 		dprintf( "Block %d already completed\n", header->block_id );
		return CONTINUE;
	}

	dprintf( "header = {\n"
			"    block_id = %d\n"
			"    packet_id = %d\n"
			"    packets_count = %d\n"
			"    retry_id = %d\n"
			"    retries_count = %d\n"
			"}\n", header->block_id, header->packet_id, header->packets_count, header->retry_id, header->retries_count );

	*pHeader = header;
	*pPayload = pu8Payload;
	*valid = is_valid;
	return bytes;
}


int process_packet( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* pret, uint32_t retmax, uint32_t* valid )
{
	// Advanced mode
	if ( rwifi->recv_mode == RAWWIFI_RX_FEC_WEIGHTED ) {
		return process_packet_weighted( rwifi, rpcap, pret, retmax, valid );
	}

	if ( _rawwifi_get_tick() - rwifi->recv_perf_last_tick >= 1000 * 1000 ) {
		uint32_t den = rwifi->recv_last_returned - rwifi->recv_perf_last_index;
		if ( den == 0 ) {
			rwifi->recv_quality = 0;
		} else {
			rwifi->recv_quality = 100 * ( rwifi->recv_perf_valid * 4 + rwifi->recv_perf_invalid ) / ( den * 4 );
			if ( rwifi->recv_quality > 100 ) {
				rwifi->recv_quality = 100;
			}
		}
		rwifi->recv_perf_last_tick = _rawwifi_get_tick();
		rwifi->recv_perf_last_index = rwifi->recv_last_returned;
		rwifi->recv_perf_valid = 0;
		rwifi->recv_perf_invalid = 0;
	}

	wifi_packet_header_t* header = 0;
	uint8_t* payload = 0;
	uint32_t is_valid = 0;
	int32_t bytes = analyze_packet( rwifi, rpcap, &header, &payload, &is_valid );
	if ( bytes < 0 ) {
		*valid = is_valid;
		return bytes;
	}

	if ( rwifi->recv_block && header->block_id > rwifi->recv_block->id ) {
		// TODO : return rwifi->recv_block
		free( rwifi->recv_block );
		rwifi->recv_block = NULL;
	}

	if ( header->packets_count == 1 && ( is_valid || header->retry_id >= header->retries_count - 1 ) ) {
		memcpy( pret, payload, bytes );
		*valid = is_valid;
		rwifi->recv_last_returned = header->block_id;
		rwifi->recv_perf_valid++;
		return bytes;
	}

	if ( !rwifi->recv_block ) {
		rwifi->recv_block = (rawwifi_block_t*)malloc( sizeof(rawwifi_block_t) );
		memset( rwifi->recv_block, 0, sizeof(rawwifi_block_t) );
		rwifi->recv_block->id = header->block_id;
		rwifi->recv_block->packets_count = header->packets_count;
	}
	rawwifi_block_t* block = rwifi->recv_block;

	if ( block->packets[header->packet_id].size == 0 || block->packets[header->packet_id].valid == 0 ) {
		memcpy( block->packets[header->packet_id].data, payload, bytes );
		block->packets[header->packet_id].size = bytes;
		block->packets[header->packet_id].valid = is_valid;
		dprintf( "setted data to %p\n", block->packets[header->packet_id].data );
	} else {
		dprintf( "packet already valid\n" );
	}


	int block_ok = 1;
	for ( uint32_t i = 0; i < block->packets_count; i++ ) {
		if ( block->packets[i].size == 0 ) {
			dprintf( "====> Block %d not ok : packet %d is empty\n", block->id, i );
			block_ok = 0;
			break;
		}
	}
	if ( block_ok == 0 && ( header->packet_id >= block->packets_count - 1 || block->packets[block->packets_count-1].size > 0 ) ) {
		dprintf( "========> Block %d half empty, deal with it <========\n", block->id );
		block_ok = 1;
	}
	if ( block_ok ) {
		uint32_t all_valid = 0;
		uint32_t offset = 0;
		for ( uint32_t i = 0; i < header->packets_count; i++ ) {
			if ( block->packets[i].size > 0 ) {
				dprintf( "memcpy( %p, %p, %u ) [%s]\n", pret + offset, block->packets[i].data, block->packets[i].size, block->packets[i].valid ? "valid" : "invalid" );
				if ( block->packets[i].data ) {
					memcpy( pret + offset, block->packets[i].data, block->packets[i].size );
				}
				offset += block->packets[i].size;
				all_valid += ( block->packets[i].valid != 0 );
			} else {
				dprintf( "leak (%d)\n", block->packets[i].size );
				if ( i < header->packets_count - 1 ) {
					offset += MAX_USER_PACKET_LENGTH - _rawwifi_headers_length;
				}
			}
		}
		block->valid = ( all_valid == header->packets_count );
		if ( block->valid ) {
			rwifi->recv_perf_valid++;
		} else {
			rwifi->recv_perf_invalid++;
		}
		*valid = block->valid;
		dprintf( "block %d %s : %d bytes total\n", block->id, block->valid ? "valid":"invalid", offset );
		free( rwifi->recv_block );
		rwifi->recv_block = NULL;
		rwifi->recv_last_returned = header->block_id;
		return offset;
	}

	return CONTINUE;
}


int rawwifi_recv( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t* valid )
{
	int ret = 0;

	do {
		ret = process_packet( rwifi, rwifi->in, data, datalen, valid );
		if ( ret == CONTINUE ) {
// 			dprintf( "rawwifi_recv, continue..\n" );
		}
	} while ( ret == CONTINUE );

	return ret;
}


uint64_t _rawwifi_get_tick()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}
