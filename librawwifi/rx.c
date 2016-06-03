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
#define dprintf printf
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

uint64_t GetTicks();

int process_frame( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* payloadBuffer, uint8_t** ppu8Payload )
{
	struct pcap_pkthdr* ppcapPacketHeader = NULL;
	struct ieee80211_radiotap_iterator rti;
	PENUMBRA_RADIOTAP_DATA prd;
	int bytes;
// 	int n;
	int retval;
	int u16HeaderLen;

	uint8_t* pu8Payload = payloadBuffer;

	retval = pcap_next_ex( rpcap->pcap, &ppcapPacketHeader, (const uint8_t**)&pu8Payload );

	if ( retval < 0 ) {
		fprintf( stderr, "pcap_next_ex ERROR : %s\n", pcap_geterr( rpcap->pcap ) );
		exit(1);
		return 0;
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
/*
	int link_update = 0;
	while ( ( n = ieee80211_radiotap_iterator_next(&rti) ) == 0 ) {
		switch ( rti.this_arg_index ) {
			case IEEE80211_RADIOTAP_RATE:
				rwifi->recv_link.rate = (*rti.this_arg);
				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_CHANNEL:
				rwifi->recv_link.channel = le16_to_cpu(*((u16 *)rti.this_arg));
	// 			prd.m_nChannelFlags = le16_to_cpu(*((u16 *)(rti.this_arg + 2)));
				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_ANTENNA:
				rwifi->recv_link.antenna = (*rti.this_arg) + 1;
				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_FLAGS:
	// 			prd.m_nRadiotapFlags = *rti.this_arg;
				link_update = 1;
				break;

			case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
				rwifi->recv_link.signal = (int8_t)(*rti.this_arg);
				link_update = 1;
				break;

			default:
				break;
		}
	}
*/
	pu8Payload += u16HeaderLen + rwifi->n80211HeaderLength;

	if ( prd.m_nRadiotapFlags & IEEE80211_RADIOTAP_F_FCS ) {
// 		bytes -= 4;
	}

	int checksum_correct = ( prd.m_nRadiotapFlags & 0x40 ) == 0;
	if ( !checksum_correct ) {
		// TODO/TBD
	}

	*ppu8Payload = pu8Payload;
	return bytes;
}


int process_packet( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* pret, uint32_t retmax, uint32_t* valid )
{
	uint8_t payloadBuffer[4096];
	uint8_t* pu8Payload = payloadBuffer;

	int32_t bytes = process_frame( rwifi, rpcap, payloadBuffer, &pu8Payload );
	if ( bytes <= 0 ) {
		return bytes;
	}

	// Clear blocks which are too old
	uint64_t ticks = GetTicks();
	rawwifi_block_t* clear_block = rwifi->recv_block;
	while ( clear_block ) {
		rawwifi_block_t* prev = clear_block->prev;
		if ( ticks - clear_block->ticks >= 1000000ULL ) {
			if ( clear_block == rwifi->recv_block ) {
				rwifi->recv_block = 0;
			} else {
				if ( clear_block->next ) {
					clear_block->next->prev = clear_block->prev;
				}
				if ( clear_block->prev ) {
					clear_block->prev->next = clear_block->next;
				}
			}
			free( clear_block );
		}
		clear_block = prev;
	}

	wifi_packet_header_t header;
	memcpy( &header, pu8Payload, sizeof( header ) );
	pu8Payload += sizeof( header );
	bytes -= sizeof( header );
	dprintf( "header = {\n    block_id = %d\n    packet_id = %d\n    packets_count = %d\n}\n", header.block_id, header.packet_id, header.packets_count & 0x7FFF );

	if ( bytes <= 0 || header.packet_id >= header.packets_count || header.packet_id > 7 ) {
		dprintf( "broken packet header\n" );
		return 0;
	}

	wifi_packet_ack_t ack = {
		.block_id = header.block_id,
		.packet_id = header.packet_id,
		.valid = 0,
	};

	int is_valid = ( header.crc == rawwifi_crc32( pu8Payload, bytes ) );

	if ( header.packets_count & RETRY_ACK ) {
		ack.valid = is_valid;
		rawwifi_send_ack( rwifi, &ack );
	}

	header.packets_count &= 0x7FFF;

/*	TBD : keep it ?
	if ( header.packets_count == 1 ) {
		if ( header.block_id == rwifi->recv_last_block ) {
			dprintf( "Block %d already completed\n", header.block_id );
			return CONTINUE;
		}
		memcpy( pret, pu8Payload, bytes ); // TODO : limit to retmax
		rwifi->recv_last_block = header.block_id;
		rwifi->recv_block_valid = is_valid;
		return bytes;
	}
*/
	if ( rwifi->recv_block != 0 && header.block_id == rwifi->recv_block->id && rwifi->recv_block->valid ) {
		dprintf( "Block %d already completed (1)\n", header.block_id );
		return CONTINUE;
	}

	rawwifi_block_t* block = rwifi->recv_block;
	while ( block && block->id != header.block_id ) {
		block = block->prev;
	}
	if ( block == 0 ) {
// 		if ( rwifi->recv_block != 0 && header.block_id > rwifi->recv_block->id ) {
// 			dprintf( "Block %d already completed (2)\n", header.block_id );
// 			return CONTINUE;
// 		}
		dprintf( "On new block %d\n", header.block_id );
		block = (rawwifi_block_t*)malloc( sizeof(rawwifi_block_t) );
		memset( block, 0, sizeof(rawwifi_block_t) );
		block->ticks = GetTicks();
		block->prev = rwifi->recv_block;
		if ( rwifi->recv_block ) {
			rwifi->recv_block->next = block;
		}
		rwifi->recv_block = block;
	}
	block->id = header.block_id;

	memcpy( block->packets[header.packet_id].data, pu8Payload, bytes );
	block->packets[header.packet_id].size = bytes;
	block->packets[header.packet_id].valid = is_valid;

	int block_ok = 1;
	for ( uint32_t i = 0; i < header.packets_count; i++ ) {
		if ( block->packets[i].size == 0 ) {
			block_ok = 0;
			break;
		}
	}
	if ( block_ok ) {
		uint32_t all_valid = 0;
		uint32_t offset = 0;
		for ( uint32_t i = 0; i < header.packets_count; i++ ) {
			if ( block->packets[i].size <= 1500 && block->packets[i].size > 0 ) {
				dprintf( "memcpy( %p, %p, %u )\n", pret + offset, block->packets[i].data, block->packets[i].size );
				memcpy( pret + offset, block->packets[i].data, block->packets[i].size );
				offset += block->packets[i].size;
				all_valid += ( block->packets[i].valid != 0 );
			} else {
				dprintf( "leak (%d)\n", block->packets[i].size );
			}
		}
		block->valid = ( all_valid == header.packets_count );
		*valid = block->valid;
		return offset;
	}

	return 0;
}


int rawwifi_recv( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t* valid )
{
	int ret = 0;

	do {
		ret = process_packet( rwifi, rwifi->in, data, datalen, valid );
	} while ( ret == CONTINUE );

	return ret;
}


int32_t rawwifi_recv_ack( rawwifi_t* rwifi, wifi_packet_ack_t* ack )
{
	uint8_t payloadBuffer[4096];
	uint8_t* pu8Payload = payloadBuffer;

// 	if ( ( ret = pcap_set_timeout( rwifi->out_ack->pcap, 1000 ) != 0 ) ) {
// 		dprintf( "pcap_set_timeout() failed %d\n", ret );
// 	}
	int32_t bytes = process_frame( rwifi, rwifi->out_ack, payloadBuffer, &pu8Payload );
	if ( bytes <= 0 ) {
		return bytes;
	}

	memcpy( ack, pu8Payload, sizeof( wifi_packet_ack_t ) );
	pu8Payload += sizeof( wifi_packet_ack_t );
	bytes -= sizeof( wifi_packet_ack_t );
	dprintf( "ACK = {\n    block_id = %d\n    packet_id = %d\n    valid = %d\n}\n", ack->block_id, ack->packet_id, ack->valid );

	return sizeof( wifi_packet_ack_t );
}

uint64_t GetTicks()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}
