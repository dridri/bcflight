#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <time.h>

#include "fec.h"

#include "lib.h"
#include "wifibroadcast.h"


static const uint8_t u8aRadiotapHeader[] = {

	0x00, 0x00, // <-- radiotap version
	0x0c, 0x00, // <- radiotap header lengt
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

static int param_data_packets_per_block = 2;
static int param_fec_packets_per_block = 1;


void set_port_no( uint8_t* pu, uint8_t port )
{
	//dirty hack: the last byte of the mac address is the port number. this makes it easy to filter out specific ports via wireshark
	pu[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = port;
	pu[sizeof(u8aRadiotapHeader) + sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*6 + sizeof(uint8_t)*5 ] = port;
}


int packet_header_init( uint8_t* packet_header )
{
	uint8_t* pu8 = packet_header;

	memcpy( packet_header, u8aRadiotapHeader, sizeof( u8aRadiotapHeader ) );
	pu8 += sizeof( u8aRadiotapHeader );

	memcpy(pu8, u8aIeeeHeader, sizeof( u8aIeeeHeader ) );
	pu8 += sizeof( u8aIeeeHeader );

	//determine the length of the header
	return pu8 - packet_header;
}


void pb_transmit_packet( pcap_t* ppcap, int seq_nr, uint8_t* packet_transmit_buffer, int packet_header_len, const uint8_t* packet_data, int packet_length )
{
	//add header outside of FEC
	wifi_packet_header_t* wph = (wifi_packet_header_t*)(packet_transmit_buffer + packet_header_len);
	wph->sequence_number = seq_nr;

	//copy data
	memcpy( packet_transmit_buffer + packet_header_len + sizeof(wifi_packet_header_t), packet_data, packet_length );

// 	printf( "send %d bytes [%d]\n", packet_length, wph->sequence_number );
	int plen = packet_length + packet_header_len + sizeof( wifi_packet_header_t );
	int r = pcap_inject( ppcap, packet_transmit_buffer, plen );
	if (r != plen) {
		pcap_perror( ppcap, "Trouble injecting packet" );
		printf( "sent %d / %d\n", r, plen );
		exit(1);
	}
}




void pb_transmit_block(packet_buffer_t* pbl, pcap_t* ppcap, int *seq_nr, int port, int packet_length, uint8_t* packet_transmit_buffer, int packet_header_len, int data_packets_per_block, int fec_packets_per_block, int transmission_count) {
	int i;
	uint8_t* data_blocks[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
	uint8_t fec_pool[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK][MAX_USER_PACKET_LENGTH];
	uint8_t* fec_blocks[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];


	for(i=0; i<data_packets_per_block; ++i) {
		data_blocks[i] = pbl[i].data;
	}


	if(fec_packets_per_block) {
		for(i=0; i<fec_packets_per_block; ++i) {
			fec_blocks[i] = fec_pool[i];
		}

		fec_encode(packet_length, data_blocks, data_packets_per_block, (unsigned char **)fec_blocks, fec_packets_per_block);
	}

	uint8_t* pb = packet_transmit_buffer;
	set_port_no(pb, port);
	pb += packet_header_len;

	int x;
	for(x=0; x<transmission_count; ++x) {
		//send data and FEC packets interleaved
		int di = 0; 
		int fi = 0;
		int seq_nr_tmp = *seq_nr;
		while(di < data_packets_per_block || fi < fec_packets_per_block) {
			if(di < data_packets_per_block) {
				pb_transmit_packet(ppcap, seq_nr_tmp, packet_transmit_buffer, packet_header_len, data_blocks[di], packet_length);
				seq_nr_tmp++;
				di++;
			}

			if(fi < fec_packets_per_block) {
				pb_transmit_packet(ppcap, seq_nr_tmp, packet_transmit_buffer, packet_header_len, fec_blocks[fi], packet_length);
				seq_nr_tmp++;
				fi++;
			}	
		}
	}

	*seq_nr += data_packets_per_block + fec_packets_per_block;

	//reset the length back
	for(i=0; i< data_packets_per_block; ++i) {
			pbl[i].len = 0;
	}
}


rwifi_tx_t* rwifi_tx_init( const char* device, int port, int blocking )
{
	int j;
	char szErrbuf[PCAP_ERRBUF_SIZE];

	rwifi_tx_t* rwifi = (rwifi_tx_t*)malloc( sizeof(rwifi_tx_t) );
	memset( rwifi, 0, sizeof( rwifi_tx_t ) );
	rwifi->port = port;

	rwifi->packet_header_length = packet_header_init( rwifi->packet_transmit_buffer );
	fec_init();

	szErrbuf[0] = '\0';
	rwifi->ppcap = pcap_open_live( device, 800, 1, 20, szErrbuf );
	if ( rwifi->ppcap == NULL ) {
		printf( "rwifi_init_tx : Unable to open interface %s in pcap: %s\n",  device, szErrbuf );
		return NULL;
	}

	pcap_setnonblock( rwifi->ppcap, !blocking, szErrbuf );

	rwifi->curr_pb = 0;
	rwifi->pbl = lib_alloc_packet_buffer_list( param_data_packets_per_block, MAX_PACKET_LENGTH * 4 );

	//prepare the buffers with headers
	for ( j = 0; j < param_data_packets_per_block; j++ ) {
		rwifi->pbl[j].len = 0;
	}

	return rwifi;
}


int rwifi_tx_send( rwifi_tx_t* rwifi, uint8_t* data, uint32_t datalen )
{
	int sent = 0;
	int len = 0;

	while ( sent < datalen ) {
		packet_buffer_t* pb = rwifi->pbl + rwifi->curr_pb;
		if ( pb->len == 0 ) {
			pb->len += sizeof( payload_header_t );
		}

		if ( sizeof( uint32_t ) + ( datalen - sent ) > MAX_USER_PACKET_LENGTH ) {
			len = MAX_USER_PACKET_LENGTH;
		} else {
			len = ( datalen - sent );
		}
		if ( pb->len + len > MAX_USER_PACKET_LENGTH ) {
			len = MAX_USER_PACKET_LENGTH - pb->len;
		}
		memcpy( pb->data + pb->len, data + sent, len );
		pb->len += len;

		if ( pb->len > 0 ) {
			payload_header_t* ph = (payload_header_t*)pb->data;
			ph->data_length = pb->len - sizeof(payload_header_t);
// 			printf( "sending %d bytes\n", ph->data_length );
			if ( rwifi->curr_pb >= param_data_packets_per_block - 1 ) {
				pb_transmit_block( rwifi->pbl, rwifi->ppcap, &rwifi->seq_nr, rwifi->port, MAX_USER_PACKET_LENGTH, rwifi->packet_transmit_buffer, rwifi->packet_header_length, param_data_packets_per_block, param_fec_packets_per_block, 1 );
				rwifi->curr_pb = 0;
			} else {
				rwifi->curr_pb++;
			}
		}

		sent += len;
	}
/*
	int len = sizeof( uint32_t ) + datalen;
	uint8_t packet_data[ MAX_PACKET_LENGTH ];

	packet_buffer_t packet = {
		.valid = 0,
		.crc_correct = 0,
		.len = len,
		.data = packet_data
	};

	*((uint32_t*)packet_data) = datalen;
	memcpy( packet_data + sizeof( uint32_t ), data, datalen );

	pb_transmit_block( &packet, rwifi->ppcap, &rwifi->seq_nr, rwifi->port, datalen, rwifi->packet_transmit_buffer, rwifi->packet_header_length, 1, 1, 1 );

	return datalen;
*/
	return sent;
}
