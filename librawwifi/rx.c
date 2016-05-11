#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
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

typedef struct  {
	int m_nChannel;
	int m_nChannelFlags;
	int m_nRate;
	int m_nAntenna;
	int m_nRadiotapFlags;
} __attribute__((packed)) PENUMBRA_RADIOTAP_DATA;

int process_frame( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* payloadBuffer, uint8_t** ppu8Payload )
{
	struct pcap_pkthdr* ppcapPacketHeader = NULL;
	struct ieee80211_radiotap_iterator rti;
	PENUMBRA_RADIOTAP_DATA prd;
	int bytes;
	int n;
	int retval;
	int u16HeaderLen;

	uint8_t* pu8Payload = payloadBuffer;

	retval = pcap_next_ex( rpcap->pcap, &ppcapPacketHeader, (const uint8_t**)&pu8Payload );

	if ( retval < 0 ) {
		fprintf( stderr, "Socket broken\n" );
		fprintf( stderr, "%s\n", pcap_geterr( rpcap->pcap ) );
		exit(1);
	}

	if ( retval != 1 ) {
		if ( rpcap->blocking == 1 ) {
			printf( "retval = %d !!\n", retval );
		}
		return 0;
	}


	u16HeaderLen = (pu8Payload[2] + (pu8Payload[3] << 8));

	if ( ppcapPacketHeader->len < ( u16HeaderLen + rwifi->n80211HeaderLength ) ) {
		return 0;
	}

	bytes = ppcapPacketHeader->len - ( u16HeaderLen + rwifi->n80211HeaderLength );
	if ( bytes < 0 ) {
		return 0;
	}

	if ( ieee80211_radiotap_iterator_init( &rti, (struct ieee80211_radiotap_header *)pu8Payload, ppcapPacketHeader->len ) < 0 ) {
		return 0;
	}

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

	pu8Payload += u16HeaderLen + rwifi->n80211HeaderLength;

	if ( prd.m_nRadiotapFlags & IEEE80211_RADIOTAP_F_FCS ) {
		bytes -= 4;
	}

	int checksum_correct = ( prd.m_nRadiotapFlags & 0x40 ) == 0;
	if ( !checksum_correct ) {
		// TODO/TBD
	}

	*ppu8Payload = pu8Payload;
	return bytes;
}


int process_packet( rawwifi_t* rwifi, rawwifi_pcap_t* rpcap, uint8_t* pret, uint32_t retmax )
{
	uint8_t payloadBuffer[4096];
	uint8_t* pu8Payload = payloadBuffer;

	int32_t bytes = process_frame( rwifi, rpcap, payloadBuffer, &pu8Payload );
	if ( bytes <= 0 ) {
		return bytes;
	}

	wifi_packet_header_t header;
	memcpy( &header, pu8Payload, sizeof( header ) );
	pu8Payload += sizeof( header );
	bytes -= sizeof( header );
	printf( "header = {\n    block_id = %d\n    packet_id = %d\n    packets_count = %d\n}\n", header.block_id, header.packet_id, header.packets_count & 0x7FFF );

	wifi_packet_ack_t ack = {
		.block_id = header.block_id,
		.packet_id = header.packet_id,
		.valid = 0,
	};

	if ( header.packets_count & RETRY_ACK ) {
		ack.valid = ( header.crc == rawwifi_crc32( pu8Payload, bytes ) );
		rawwifi_send_ack( rwifi, &ack );
	}

	header.packets_count &= 0x7FFF;

	if ( header.packets_count == 1 ) {
		if ( header.block_id == rwifi->recv_last_block ) {
			printf( "Block %d already completed\n", header.block_id );
			return CONTINUE;
		}
		memcpy( pret, pu8Payload, bytes ); // TODO : limit to retmax
		rwifi->recv_last_block = header.block_id;
		return bytes;
	}

	if ( header.block_id != rwifi->recv_block.id ) {
		printf( "On new block %d\n", header.block_id );
		memset( &rwifi->recv_block, 0, sizeof( rwifi->recv_block ) );
	}
	rwifi->recv_block.id = header.block_id;

	memcpy( rwifi->recv_block.packets[header.packet_id].data, pu8Payload, bytes );
	rwifi->recv_block.packets[header.packet_id].size = bytes;

	int block_ok = 1;
	for ( uint32_t i = 0; i < header.packets_count; i++ ) {
		if ( rwifi->recv_block.packets[i].size == 0 ) {
			block_ok = 0;
			break;
		}
	}
	if ( block_ok ) {
		if ( header.block_id == rwifi->recv_last_block ) {
			printf( "Block %d already completed\n", header.block_id );
			return CONTINUE;
		}
		uint32_t offset = 0;
		for ( uint32_t i = 0; i < header.packets_count; i++ ) {
			memcpy( pret + offset, rwifi->recv_block.packets[i].data, rwifi->recv_block.packets[i].size );
			offset += rwifi->recv_block.packets[i].size;
		}
		rwifi->recv_last_block = header.block_id;
		return offset;
	}

	return 0;
}


int rawwifi_recv( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen )
{
	int ret = 0;

	do {
		ret = process_packet( rwifi, rwifi->in, data, datalen );
	} while ( ret == CONTINUE );

	return ret;
}


int32_t rawwifi_recv_ack( rawwifi_t* rwifi, wifi_packet_ack_t* ack )
{
	uint8_t payloadBuffer[4096];
	uint8_t* pu8Payload = payloadBuffer;

	pcap_set_timeout( rwifi->out_ack->pcap, 50 );
	int32_t bytes = process_frame( rwifi, rwifi->out_ack, payloadBuffer, &pu8Payload );
	if ( bytes <= 0 ) {
		return bytes;
	}

	memcpy( ack, pu8Payload, sizeof( wifi_packet_ack_t ) );
	pu8Payload += sizeof( wifi_packet_ack_t );
	bytes -= sizeof( wifi_packet_ack_t );
	printf( "ACK = {\n    block_id = %d\n    packet_id = %d\n    valid = %d\n}\n", ack->block_id, ack->packet_id, ack->valid );

	return sizeof( wifi_packet_ack_t );
}
