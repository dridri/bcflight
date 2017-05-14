#include <stdlib.h>
#include <string.h>
//#include <iwlib.h>
#include "rawwifi.h"

extern const uint32_t rawwifi_headers_length;
static pthread_mutex_t compile_mutex = PTHREAD_MUTEX_INITIALIZER; 

static rawwifi_pcap_t* setup_tx( rawwifi_t* rwifi, int port, int blocking )
{
	rawwifi_pcap_t* rpcap = (rawwifi_pcap_t*)malloc( sizeof(rawwifi_pcap_t) );
	char szErrbuf[PCAP_ERRBUF_SIZE];
	memset( szErrbuf, 0, sizeof( szErrbuf ) );

	rpcap->pcap = pcap_create( rwifi->device, szErrbuf );
	if ( rpcap->pcap == NULL ) {
		printf( "rawwifi_init : Unable to open interface %s in pcap_out: %s\n",  rwifi->device, szErrbuf );
		return NULL;
	}
	pcap_set_snaplen( rpcap->pcap, 800 );
	pcap_set_promisc( rpcap->pcap, 1 );
#ifndef WIN32
	pcap_set_immediate_mode( rpcap->pcap, 1 );
#endif
	if ( pcap_activate( rpcap->pcap ) != 0 ) {
		printf( "Unable to open interface %s in pcap: %s\n", rwifi->device, szErrbuf );
		return NULL;
	}

	if ( pcap_setnonblock( rpcap->pcap, !blocking, szErrbuf ) < 0 ) {
		printf( "Error setting %s output to %s mode: %s\n", rwifi->device, blocking ? "blocking" : "non blocking", szErrbuf );
	}

	rpcap->port = port;
	rpcap->blocking = blocking;

	return rpcap;
}


static rawwifi_pcap_t* setup_rx( rawwifi_t* rwifi, int port, int blocking, int read_timeout_ms )
{
	rawwifi_pcap_t* rpcap = (rawwifi_pcap_t*)malloc( sizeof(rawwifi_pcap_t) );
	char szErrbuf[PCAP_ERRBUF_SIZE];
	char szProgram[512] = "";
	struct bpf_program bpfprogram;
	memset( szErrbuf, 0, sizeof( szErrbuf ) );

	rpcap->pcap = pcap_create( rwifi->device, szErrbuf );
	if ( rpcap->pcap == NULL ) {
		printf( "Unable to open interface %s in pcap: %s\n", rwifi->device, szErrbuf );
		return NULL;
	}
	pcap_set_snaplen( rpcap->pcap, 2048 );
	pcap_set_promisc( rpcap->pcap, 1 );
#ifndef WIN32
	pcap_set_immediate_mode( rpcap->pcap, 1 );
#endif
	pcap_set_timeout( rpcap->pcap, read_timeout_ms );
	if ( pcap_activate( rpcap->pcap ) != 0 ) {
		printf( "Unable to open interface %s in pcap: %s\n", rwifi->device, szErrbuf );
		return NULL;
	}

	if ( pcap_setnonblock( rpcap->pcap, !blocking, szErrbuf ) < 0 ) {
		printf( "Error setting %s input to %s mode: %s\n", rwifi->device, blocking ? "blocking" : "non blocking", szErrbuf );
	}

	switch ( pcap_datalink( rpcap->pcap ) ) {
		case DLT_PRISM_HEADER:
			printf( "DLT_PRISM_HEADER Encap\n" );
			rwifi->n80211HeaderLength = 0x20; // ieee80211 comes after this
			sprintf( szProgram, "radio[0x4a:4]==0x13223344 && radio[0x4e:2] == 0x55%.2x", port );
			break;

		case DLT_IEEE802_11_RADIO:
			printf( "DLT_IEEE802_11_RADIO Encap\n" );
			rwifi->n80211HeaderLength = 0x18; // ieee80211 comes after this
			sprintf( szProgram, "ether[0x0a:4]==0x13223344 && ether[0x0e:2] == 0x55%.2x", port );
// 			sprintf( szProgram, "ether[0x0a:4]==0xb827ebc7 && ether[0x0e:2] == 0x6b75" ); // b8:27:eb:c7:6b:75
			break;

		default:
			printf( "!!! unknown encapsulation on %s !\n", rwifi->device );
			return NULL;
	}

	printf( "pcap_in program : %s\n", szProgram ); fflush(stdout);

	int i = 0;
	int ok = 0;
	for ( i = 0; i < 8; i++ ) {
		if ( pcap_compile( rpcap->pcap, &bpfprogram, szProgram, 1, 0 ) >= 0 ) {
			ok = 1;
			break;
		}
	}
	if ( !ok ) {
		printf( "PCAP ERROR 1 : %s\n", pcap_geterr( rpcap->pcap ) ); fflush(stdout);
		return NULL;
	}
	ok = 0;
	for ( i = 0; i < 8; i++ ) {
		if ( pcap_setfilter( rpcap->pcap, &bpfprogram ) >= 0 ) {
			ok = 1;
			break;
		}
	}
	if ( !ok ) {
		printf( "PCAP ERROR 2 : %s\n", pcap_geterr( rpcap->pcap ) ); fflush(stdout);
		return NULL;
	}
	pcap_freecode( &bpfprogram );

	rpcap->port = port;
	rpcap->blocking = blocking;

	return rpcap;
}


rawwifi_t* rawwifi_init( const char* device, int rx_port, int tx_port, int blocking, int read_timeout_ms )
{
	pthread_mutex_lock( &compile_mutex );
	rawwifi_t* rwifi = (rawwifi_t*)malloc( sizeof(rawwifi_t) );
	memset( rwifi, 0, sizeof( rawwifi_t ) );

	rwifi->device = strdup( device );
	rwifi->iw_socket = -1;//iw_sockets_open();

	rwifi->in = setup_rx( rwifi, tx_port, blocking, read_timeout_ms );
	rwifi->out = setup_tx( rwifi, rx_port, blocking );
	rwifi->recv_timeout_ms = read_timeout_ms;
	rwifi->recv_mode = RAWWIFI_RX_FAST;
	rwifi->recv_recover = RAWWIFI_FILL_WITH_ZEROS;

	if ( !rwifi->out || !rwifi->in ) {
		pthread_mutex_unlock( &compile_mutex );
		return NULL;
	}

	rawwifi_init_txbuf( rwifi->tx_buffer );
	pthread_mutex_unlock( &compile_mutex );
	return rwifi;
}


void rawwifi_set_recv_mode( rawwifi_t* rwifi, RAWWIFI_RX_FEC_MODE mode )
{
	if ( rwifi != 0 ) {
		rwifi->recv_mode = mode;
	}
}


void rawwifi_set_recv_block_recover_mode( rawwifi_t* rwifi, RAWWIFI_BLOCK_RECOVER_MODE mode )
{
	if ( rwifi != 0 ) {
		rwifi->recv_recover = mode;
	}
}


int32_t rawwifi_recv_quality( rawwifi_t* rwifi )
{
	if ( rwifi == 0 ) {
		return 0;
	}
	return rwifi->recv_quality;
}


int32_t rawwifi_recv_level( rawwifi_t* rwifi )
{
	if ( rwifi == 0 ) {
		return 0;
	}
	return rwifi->recv_link.signal;
}


uint32_t rawwifi_recv_speed( rawwifi_t* rwifi )
{
	if ( rwifi == 0 ) {
		return 0;
	}
	return rwifi->recv_perf_speed;
}


uint32_t rawwifi_send_headers_length( rawwifi_t* rwifi )
{
	(void)rwifi;
	return rawwifi_headers_length;
}


void rawwifi_set_send_max_block_size( rawwifi_t* rwifi, uint32_t max_block_size )
{
	if ( rwifi != 0 ) {
		rwifi->max_block_size = max_block_size;
	}
}


void rawwifi_set_send_block_flags( rawwifi_t* rwifi, RAWWIFI_BLOCK_FLAGS flags )
{
	if ( rwifi != 0 ) {
		rwifi->send_block_flags = flags;
	}
}


uint32_t rawwifi_crc32( const uint8_t* buf, uint32_t len )
{
	uint32_t k = 0;
	uint32_t crc = 0;

	crc = ~crc;

	while ( len-- ) {
		crc ^= *buf++;
		for ( k = 0; k < 8; k++ ) {
			crc = ( crc & 1 ) ? ( (crc >> 1) ^ 0x82f63b78 ) : ( crc >> 1 );
		}
	}

	return ~crc;
}


uint16_t rawwifi_crc16( const uint8_t* buf, uint32_t len )
{
	uint8_t x;
	uint16_t crc = 0xFFFF;

	while ( len-- ) {
		x = crc >> 8 ^ *buf++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
	}

	return crc;
}
