#pragma once



#include <stdint.h>
#include <stdlib.h>

#include "wifibroadcast.h"

#define MAX_PACKET_LENGTH 4192
#define MAX_USER_PACKET_LENGTH 1450
#define MAX_DATA_OR_FEC_PACKETS_PER_BLOCK 32

typedef struct {
	uint32_t received_packet_cnt;
	uint32_t wrong_crc_cnt;
	int8_t current_signal_dbm;
} wifi_adapter_rx_status_t;

typedef struct {
	time_t last_update;
	uint32_t received_block_cnt;
	uint32_t damaged_block_cnt;
	uint32_t tx_restart_cnt;

	uint32_t wifi_adapter_cnt;
	wifi_adapter_rx_status_t adapter[MAX_PENUMBRA_INTERFACES];
} wifibroadcast_rx_status_t;


//this sits at the payload of the wifi packet (outside of FEC)
typedef struct {
    uint32_t sequence_number;
} __attribute__((packed)) wifi_packet_header_t;

//this sits at the data payload (which is usually right after the wifi_packet_header_t) (inside of FEC)
typedef struct {
    uint32_t data_length;
} __attribute__((packed)) payload_header_t;


packet_buffer_t *lib_alloc_packet_buffer_list(size_t num_packets, size_t packet_length);
