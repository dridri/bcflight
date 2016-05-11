#include <sys/types.h>
#include <stdint.h>
#include <pcap.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_USER_PACKET_LENGTH 1450
#define RETRY_ACK 0x8000

typedef struct {
	uint32_t block_id;
	uint16_t packet_id;
	uint16_t packets_count;
	uint32_t crc;
} __attribute__((packed)) wifi_packet_header_t;

typedef struct {
	uint32_t block_id;
	uint16_t packet_id;
	uint16_t valid;
} __attribute__((packed)) wifi_packet_ack_t;


typedef struct rawwifi_packet_t {
	uint8_t data[2048];
	uint32_t size;
} rawwifi_packet_t;


typedef struct rawwifi_block_t {
	uint32_t id;
	rawwifi_packet_t packets[8];
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
	rawwifi_pcap_t* out_ack;
	uint32_t send_block_id;
	rawwifi_link_t send_link;

	// Receive
	rawwifi_pcap_t* in;
	rawwifi_pcap_t* in_ack;
	rawwifi_block_t recv_block;
	uint32_t recv_last_block;
	rawwifi_link_t recv_link;
} rawwifi_t;


rawwifi_t* rawwifi_init( const char* device, int rx_port, int tx_port, int blocking );

int rawwifi_send( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen );
int rawwifi_send_retry( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen, uint32_t retries );
int rawwifi_recv( rawwifi_t* rwifi, uint8_t* data, uint32_t datalen );

// internals
int32_t rawwifi_recv_ack( rawwifi_t* rwifi, wifi_packet_ack_t* ack );
int32_t rawwifi_send_ack( rawwifi_t* rwifi, wifi_packet_ack_t* ack );
uint32_t rawwifi_crc32( const uint8_t* data, uint32_t len );

#ifdef __cplusplus
}
#endif
