/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <pcap.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_USER_PACKET_LENGTH 1450 // wifi max : 1450
#define MAX_PACKET_PER_BLOCK 64

typedef struct {
	uint32_t block_id;
	uint8_t packet_id;
	uint8_t packets_count;
	uint8_t retry_id;
	uint8_t retries_count;
	uint32_t crc;
} __attribute__((packed)) wifi_packet_header_t;


typedef struct rawwifi_packet_t {
	uint8_t data[2048];
	uint32_t size;
	uint32_t valid;
} rawwifi_packet_t;


typedef struct rawwifi_block_t {
	uint32_t id;
	uint64_t ticks;
	uint16_t valid;
	uint16_t packets_count;
	rawwifi_packet_t packets[MAX_PACKET_PER_BLOCK];
} rawwifi_block_t;


typedef struct rawwifi_link_t {
	int32_t rate;
	int32_t channel;
	int32_t antenna;
	int32_t signal;
	int32_t noise;
	int32_t quality;
} rawwifi_link_t;


typedef struct rawwifi_pcap_t {
	pcap_t* pcap;
	int port;
	int blocking;
} rawwifi_pcap_t;

 
typedef struct rawwifi_t {
	char* device;
	int iw_socket;
	int n80211HeaderLength;

	// Send
	rawwifi_pcap_t* out;
	uint32_t send_block_id;
	rawwifi_link_t send_link;
	uint8_t tx_buffer[4096];

	// Receive
	rawwifi_pcap_t* in;
	rawwifi_block_t* recv_block;
	rawwifi_link_t recv_link;
	int32_t recv_timeout_ms;
	uint32_t recv_last_returned;
	uint32_t recv_quality;
	uint64_t recv_perf_last_tick;
	uint32_t recv_perf_last_index;
	uint32_t recv_perf_valid;
	uint32_t recv_perf_invalid;
} rawwifi_t;


rawwifi_t* rawwifi_init( const char* device, int rx_port, int tx_port, int blocking, int read_timeout_ms );
int32_t rawwifi_recv_quality( rawwifi_t* rwifi );

int rawwifi_send( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen );
int rawwifi_send_retry( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t retries );
int rawwifi_recv( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t* valid );

// internals
void rawwifi_init_txbuf( uint8_t* buf );
uint32_t rawwifi_crc32( const uint8_t* data, uint32_t len );

rawwifi_block_t* blocks_insert_front( rawwifi_block_t** list, uint16_t packets_count );
void blocks_pop_front( rawwifi_block_t** list );
void blocks_pop( rawwifi_block_t** list, rawwifi_block_t** block );
void blocks_pop_back( rawwifi_block_t** list );

#ifdef __cplusplus
}
#endif
