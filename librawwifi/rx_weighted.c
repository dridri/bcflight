#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#define socklen_t int
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <iwlib.h>
#endif

#include "rawwifi.h"

#define CONTINUE -2

// #define DEBUG
#ifdef DEBUG
#define dprintf(...) fprintf( stderr, __VA_ARGS__ );
#else
#define dprintf(...) ;
#endif

typedef struct packet_retry_t {
	uint8_t data[2048];
	uint32_t size;
	uint32_t valid;
} packet_retry_t;

typedef struct packet_t {
	packet_retry_t retries[16];
	uint32_t nRetries;
	uint32_t lastRetry;
	uint32_t crc;
} packet_t;

typedef struct block_t {
	uint32_t id;
	uint64_t ticks;
	uint16_t valid;
	uint16_t packets_count;
	packet_t packets[MAX_PACKET_PER_BLOCK];
} block_t;

extern const uint32_t _rawwifi_headers_length;
uint64_t _rawwifi_get_tick();
int analyze_packet( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, wifi_packet_header_t** pHeader, uint8_t** pPayload, uint32_t* valid );
static int32_t reconstruct( rawwifi_t* rwifi, block_t* block, uint8_t* pret, uint32_t retmax, uint32_t* valid );
static int32_t fast_cec( packet_t* packet, uint8_t* pret, uint32_t* valid, uint32_t* quality );

int process_packet_weighted( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* pret, uint32_t retmax, uint32_t* valid )
{
	block_t* last_block = (block_t*)rwifi->recv_private;

	if ( last_block && last_block->packets_count == 1 && last_block->packets[0].lastRetry >= last_block->packets[0].nRetries - 1 ) {
		int ret = reconstruct( rwifi, last_block, pret, retmax, valid );
		rwifi->recv_last_returned = last_block->id;
		free( last_block );
		rwifi->recv_private = NULL;
		return ret;
	}

	if ( _rawwifi_get_tick() - rwifi->recv_perf_last_tick >= 1000 * 500 ) {
		uint32_t den = rwifi->recv_last_returned - rwifi->recv_perf_last_index;
		if ( den == 0 ) {
			rwifi->recv_quality = 0;
		} else {
			rwifi->recv_quality = rwifi->recv_perf_valid / den;
			if ( rwifi->recv_quality > 100 ) {
// 				rwifi->recv_quality = 100;
			}
		}
		rwifi->recv_perf_last_tick = _rawwifi_get_tick();
		rwifi->recv_perf_last_index = rwifi->recv_last_returned;
		rwifi->recv_perf_valid = 0;
		rwifi->recv_perf_invalid = 0;
		rwifi->recv_perf_speed = rwifi->recv_perf_speed_accum * 2;
		rwifi->recv_perf_speed_accum = 0;
	}


	wifi_packet_header_t* header = 0;
	uint8_t* payload = 0;
	uint32_t is_valid = 0;
	int32_t bytes = analyze_packet( rwifi, rpcap, &header, &payload, &is_valid );
	if ( bytes <= 0 || payload == 0 || header == 0 ) {
		*valid = is_valid;
		return bytes;
	}

	if ( last_block && header->block_id > last_block->id ) {
		// We got over the last block (its last packet hasn't been received)
		// So we return it with the missing data
		int ret = reconstruct( rwifi, last_block, pret, retmax, valid );
		rwifi->recv_last_returned = last_block->id;
		free( last_block );

		block_t* new_block = (block_t*)malloc( sizeof( block_t ) );
		memset( new_block, 0, sizeof( block_t ) );
		new_block->id = header->block_id;
		new_block->packets_count = header->packets_count;
		new_block->packets[header->packet_id].crc = header->crc;
		new_block->packets[header->packet_id].nRetries = header->retries_count;
		new_block->packets[header->packet_id].lastRetry = header->retry_id;
		new_block->packets[header->packet_id].retries[header->retry_id].size = bytes;
		new_block->packets[header->packet_id].retries[header->retry_id].valid = is_valid;
		memcpy( new_block->packets[header->packet_id].retries[header->retry_id].data, payload, bytes );
		rwifi->recv_private = (void*)new_block;
		return ret;
	}

	if ( header->packets_count == 1 && ( is_valid || header->retry_id >= header->retries_count - 1 ) ) {
		dprintf( "small block\n" );
		if ( bytes > retmax ) {
			bytes = retmax;
		}
		memcpy( pret, payload, bytes );
		*valid = is_valid;
		rwifi->recv_perf_valid += 100;
		if ( rwifi->recv_private ) {
			free( rwifi->recv_private );
			rwifi->recv_private = 0;
		}
		rwifi->recv_last_returned = header->block_id;
		free( header ); // Actually frees header+payload
		return bytes;
	}

	if ( rwifi->recv_private == NULL ) {
		block_t* new_block = (block_t*)malloc( sizeof( block_t ) );
		memset( new_block, 0, sizeof( block_t ) );
		new_block->id = header->block_id;
		new_block->packets_count = header->packets_count;
		rwifi->recv_private = new_block;
	}
	block_t* block = (block_t*)rwifi->recv_private;

	block->packets[header->packet_id].crc = header->crc;
	block->packets[header->packet_id].nRetries = header->retries_count;
	block->packets[header->packet_id].lastRetry = header->retry_id;
	block->packets[header->packet_id].retries[header->retry_id].size = bytes;
	block->packets[header->packet_id].retries[header->retry_id].valid = is_valid;
	memcpy( block->packets[header->packet_id].retries[header->retry_id].data, payload, bytes );

	// ( Last packet, valid CRC ) OR ( Last packet, last retry )
	if ( header->packet_id >= header->packets_count - 1 && ( is_valid || header->retry_id >= header->retries_count - 1 ) ) {
		int ret = reconstruct( rwifi, block, pret, retmax, valid );
		rwifi->recv_last_returned = block->id;
		free( block );
		rwifi->recv_private = NULL;
		free( header ); // Actually frees header+payload
		return ret;
	}

	free( header ); // Actually frees header+payload
	return CONTINUE;
}


static int32_t reconstruct( rawwifi_t* rwifi, block_t* block, uint8_t* pret, uint32_t retmax, uint32_t* valid )
{
	uint32_t all_valid = 0;
	uint32_t offset = 0;
	uint32_t is_valid = 0;
	uint32_t quality = 0;

	for ( uint32_t i = 0; i < block->packets_count; i++ ) {
		if ( offset + block->packets[i].retries[0].size >= retmax - 1 ) {
			break;
		}
		is_valid = 0;
		quality = 0;
		uint32_t ret = fast_cec( &block->packets[i], pret + offset, &is_valid, &quality );
		if ( ret == 0 ) {
			dprintf( "packet %d:%d broken\n", block->id, i );
		} else {
			dprintf( "packet %d:%d quality : %d%% [%d]\n", block->id, i, quality * 100 / ret, is_valid );
		}
		if ( ret > 0 ) {
			offset += ret;
			all_valid += ( is_valid != 0 );
		} else {
			dprintf( "leak (%d)\n", block->packets[i].retries[0].size );
			if ( i < block->packets_count - 1 && rwifi->recv_recover == RAWWIFI_FILL_WITH_ZEROS ) {
				memset( pret + offset, 0, MAX_USER_PACKET_LENGTH - _rawwifi_headers_length );
				offset += MAX_USER_PACKET_LENGTH - _rawwifi_headers_length;
			}
		}
		if ( ret > 0 && block->packets_count > 0 ) {
			rwifi->recv_perf_valid += quality * 100 / ret / block->packets_count;
		}
		if ( offset >= retmax - 1 ) {
			break;
		}
	}
	block->valid = ( all_valid == block->packets_count );
	*valid = block->valid;
	dprintf( "block %d %s : %d bytes total\n", block->id, block->valid ? "valid":"invalid", offset );
	rwifi->recv_last_returned = block->id;
	return offset;
}


static int32_t fast_cec( packet_t* packet, uint8_t* pret, uint32_t* valid, uint32_t* quality )
{
	uint32_t broken = 0;
	uint32_t b = 0;
	uint32_t i = 0;
	uint32_t r = 0;
	uint32_t size = 0;
	uint32_t ndatas = 0;
	uint8_t* datas[32] = { 0 };
	uint32_t maximum = packet->nRetries * 1000;
	uint32_t threshold = maximum >> 1;

	*quality = 0;

	for ( r = 0, ndatas = 0; r < packet->nRetries; r++ ) {
		if ( packet->retries[r].size != 0 ) {
			if ( packet->retries[r].valid ) {
				dprintf( "shortcut\n" );
				memcpy( pret, packet->retries[r].data, packet->retries[r].size );
				*quality = packet->retries[r].size;
				*valid = 1;
				return packet->retries[r].size;
			}
			if ( size == 0 ) {
				size = packet->retries[r].size;
			}
			datas[ndatas++] = packet->retries[r].data;
		} else {
			broken = 1;
		}
	}

	for ( b = 0; b < size; b++ ) {
		uint8_t byte = 0;
		for ( i = 7; (int32_t)i >= 0; i-- ) {
			uint32_t bit = 0;
			for ( r = 0; r < ndatas; r++ ) {
				bit += ( ( datas[r][b] >> i ) & 1 ) * 1000;
			}
			byte = ( byte << 1 ) | ( bit >= threshold );
			*quality += ( ( bit == 0 ) || ( bit == maximum ) );
		}
		pret[b] = byte;
	}

	*quality /= 8;
	*valid = ( broken ? 0 : ( rawwifi_crc32( pret, size ) == packet->crc ) );
	return size;
}
