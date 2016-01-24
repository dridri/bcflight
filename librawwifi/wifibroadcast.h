#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <resolv.h>
#include <string.h>
#include <utime.h>
#include <unistd.h>
#include <getopt.h>
#include <pcap.h>
#include <endian.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct {
	int valid;
	int crc_correct;
	size_t len; //this is the actual length of the packet stored in data
	uint8_t* data;
} packet_buffer_t;

typedef struct rwifi_tx_t {
	pcap_t* ppcap;
	int port;
	int seq_nr;
    uint8_t packet_transmit_buffer[16384];
	packet_buffer_t* pbl;
	int curr_pb;
	int packet_header_length;
} rwifi_tx_t;

rwifi_tx_t* rwifi_tx_init( const char* device, int port, int blocking );
int rwifi_tx_send( rwifi_tx_t* rwifi, uint8_t* data, uint32_t datalen );


typedef struct {
	int block_num;
	packet_buffer_t* packet_buffer_list;
} block_buffer_t;

typedef struct rwifi_rx_t {
	pcap_t *ppcap;
	int blocking;
	int n80211HeaderLength;
	block_buffer_t block_buffer;
} rwifi_rx_t;

rwifi_rx_t* rwifi_rx_init( const char* device, int port, int blocking );
int rwifi_rx_recv( rwifi_rx_t* rwifi, uint8_t* data, uint32_t maxlen );

#ifdef __cplusplus
};
#endif
