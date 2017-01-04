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
#define DEBUG_PORT -1
// #define DEBUG_PORT 11
#ifdef DEBUG
#define dprintf(...) if(DEBUG_PORT==-1||rpcap->port==DEBUG_PORT){fprintf( stderr, __VA_ARGS__ );}
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

static pthread_mutex_t pcap_mutex = PTHREAD_MUTEX_INITIALIZER; 

int process_frame( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t** ppu8Payload, uint32_t* valid )
{
	struct pcap_pkthdr* ppcapPacketHeader = NULL;
	struct ieee80211_radiotap_iterator rti;
	PENUMBRA_RADIOTAP_DATA prd = { 0, 0, 0, 0, 0 };
	int bytes;
	int retval;
	int u16HeaderLen;

	uint8_t* payload = 0;

	uint64_t read_ticks = _rawwifi_get_tick();
	retval = pcap_next_ex( rpcap->pcap, &ppcapPacketHeader, (const uint8_t**)&payload );
	pthread_mutex_lock( &pcap_mutex );

	if ( rwifi->recv_timeout_ms > 0 && retval == 0 && _rawwifi_get_tick() - read_ticks >= rwifi->recv_timeout_ms * 1000 ) {
		pthread_mutex_unlock( &pcap_mutex );
		return -3;
	}

	if ( retval < 0 && errno != 0 && errno != 11 ) {
		pthread_mutex_unlock( &pcap_mutex );
		fprintf( stderr, "pcap_next_ex ERROR : %s (%d - %s)\n", pcap_geterr( rpcap->pcap ), errno, strerror(errno) );
// 		char str[1024] = "";
// 		sprintf( str, "ifconfig %s up", rwifi->device );
// 		exit(1);
// 		return CONTINUE;
		return -1;
	}

	if ( retval != 1 ) {
		pthread_mutex_unlock( &pcap_mutex );
		fprintf( stderr, "pcap_next_ex ERROR : %s (%d - %s) [continue..]\n", pcap_geterr( rpcap->pcap ), errno, strerror(errno) );
		if ( rpcap->blocking == 1 ) {
			dprintf( "retval = %d !!\n", retval );
		}
		return CONTINUE;
	}

	if ( payload == 0 ) {
		pthread_mutex_unlock( &pcap_mutex );
		return CONTINUE;
	}

	u16HeaderLen = (payload[2] + (payload[3] << 8));

	if ( ppcapPacketHeader->len < ( u16HeaderLen + rwifi->n80211HeaderLength ) ) {
		pthread_mutex_unlock( &pcap_mutex );
		dprintf( "ppcapPacketHeader->len < ( u16HeaderLen + rwifi->n80211HeaderLength )\n" );
		return CONTINUE;
	}

	bytes = ppcapPacketHeader->len - ( u16HeaderLen + rwifi->n80211HeaderLength );
	if ( bytes < 0 ) {
		pthread_mutex_unlock( &pcap_mutex );
		dprintf( "bytes < 0\n" );
		return CONTINUE;
	}

	if ( ieee80211_radiotap_iterator_init( &rti, (struct ieee80211_radiotap_header *)payload, ppcapPacketHeader->len ) < 0 ) {
		pthread_mutex_unlock( &pcap_mutex );
		dprintf( "ieee80211_radiotap_iterator_init( &rti, (struct ieee80211_radiotap_header *)payload, ppcapPacketHeader->len ) < 0\n" );
		return CONTINUE;
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

	*valid = -1;
	if ( prd.m_nRadiotapFlags & IEEE80211_RADIOTAP_F_FCS ) {
		bytes -= 4;
		*valid = ( ( prd.m_nRadiotapFlags & IEEE80211_RADIOTAP_F_BADFCS ) == 0 );
	}

	*ppu8Payload = (uint8_t*)malloc( bytes * 2 ); // Double the buffer size to avoid any leaks
	memcpy( *ppu8Payload, payload + u16HeaderLen + rwifi->n80211HeaderLength, bytes );

	pthread_mutex_unlock( &pcap_mutex );

	return bytes;
}


int analyze_packet( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, wifi_packet_header_t** pHeader, uint8_t** pPayload, uint32_t* valid )
{
	uint8_t* pu8PayloadBase = 0;

	int32_t bytes = process_frame( rwifi, rpcap, &pu8PayloadBase, valid );
	if ( pu8PayloadBase == 0 ) {
		return bytes;
	}
	if ( bytes <= 0 ) {
		free( pu8PayloadBase );
		return bytes;
	}

	wifi_packet_header_t* header = (wifi_packet_header_t*)pu8PayloadBase;
	uint8_t* pu8Payload = pu8PayloadBase + sizeof( wifi_packet_header_t );
	bytes -= sizeof( wifi_packet_header_t );

	if ( header->header_crc != rawwifi_crc16( (uint8_t*)header, sizeof(wifi_packet_header_t) - sizeof(uint16_t) ) ) {
		dprintf( "Invalid header CRC, dropping ! [%d:%d]\n", header->block_id, header->packet_id );
		free( pu8PayloadBase );
		return CONTINUE;
	}

	if ( bytes <= 0 || bytes > ( MAX_USER_PACKET_LENGTH + 128 ) || header->packet_id >= header->packets_count || header->packet_id > MAX_PACKET_PER_BLOCK || header->packets_count > MAX_PACKET_PER_BLOCK ) {
		dprintf( "Header seams broken, dropping !\n" );
		*valid = 0;
		free( pu8PayloadBase );
		return CONTINUE;
	}
	if ( rwifi->recv_last_returned >= header->block_id + 32 ) {
		// More than 32 underruns, TX has probably been resetted
		rwifi->recv_last_returned = 0;
		free( rwifi->recv_block );
		rwifi->recv_block = NULL;
	}

	int is_valid = 0;
	if ( *valid == 0 ) {
		dprintf( "pcap says frame's CRC is invalid\n" );
	} else if ( *valid == 1 ) {
		dprintf( "pcap says frame's CRC is valid\n" );
		is_valid = 1;
	} else if ( (int32_t)*valid < 0 ) { // pcap doesn't know if frame is valid, so check it
		uint32_t calculated_crc = rawwifi_crc32( pu8Payload, bytes );
		is_valid = ( header->crc == calculated_crc );
		if ( is_valid == 0 ) {
			dprintf( "Invalid CRC ! (%08X != %08X)\n", header->crc, calculated_crc );
		}
	}

	if ( header->block_id <= rwifi->recv_last_returned ) {
// 		dprintf( "Block %d already completed\n", header->block_id );
		free( pu8PayloadBase );
		return CONTINUE;
	}

	dprintf( "header = {\n"
			"    block_id = %d\n"
			"    packet_id = %d\n"
			"    packets_count = %d\n"
			"    retry_id = %d\n"
			"    retries_count = %d\n"
			"}, valid = %d\n", header->block_id, header->packet_id, header->packets_count, header->retry_id, header->retries_count, is_valid );

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

	if ( _rawwifi_get_tick() - rwifi->recv_perf_last_tick >= 1000 * 500 ) {
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
	if ( bytes <= 0 || payload == 0 || header == 0 ) {
		*valid = is_valid;
		return bytes;
	}

	if ( rwifi->recv_block && header->block_id > rwifi->recv_block->id ) {
		// TODO : return rwifi->recv_block
		free( rwifi->recv_block );
		rwifi->recv_block = NULL;
	}

	if ( header->packets_count == 1 && ( is_valid || header->retry_id >= header->retries_count - 1 ) ) {
		dprintf( "small block\n" );
		if ( bytes > retmax ) {
			bytes = retmax;
		}
		memcpy( pret, payload, bytes );
		*valid = is_valid;
		rwifi->recv_last_returned = header->block_id;
		rwifi->recv_perf_valid++;
		free( header ); // Actually frees header+payload
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
				dprintf( "[%d/%d]memcpy( %p, %p, %u ) [%s]\n", i+1, header->packets_count, pret + offset, block->packets[i].data, block->packets[i].size, block->packets[i].valid ? "valid" : "invalid" );
				if ( block->packets[i].data ) {
					memcpy( pret + offset, block->packets[i].data, block->packets[i].size );
				}
				offset += block->packets[i].size;
				all_valid += ( block->packets[i].valid != 0 );
			} else {
				dprintf( "[%d/%d]leak (%d)\n", i+1, header->packets_count, block->packets[i].size );
				if ( i < header->packets_count - 1 && rwifi->recv_recover == RAWWIFI_FILL_WITH_ZEROS ) {
					memset( pret + offset, 0, MAX_USER_PACKET_LENGTH - _rawwifi_headers_length );
					offset += MAX_USER_PACKET_LENGTH - _rawwifi_headers_length;
				}
			}
			if ( offset >= retmax - 1 ) {
				break;
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
		free( header ); // Actually frees header+payload
		return offset;
	}

	free( header ); // Actually frees header+payload
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
