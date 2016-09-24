#include <stdlib.h>
#include <string.h>
#include <iwlib.h>
#include "rawwifi.h"


static rawwifi_pcap_t* setup_tx( rawwifi_t* rwifi, int port, int blocking )
{
	rawwifi_pcap_t* rpcap = (rawwifi_pcap_t*)malloc( sizeof(rawwifi_pcap_t) );
	char szErrbuf[PCAP_ERRBUF_SIZE];
	memset( szErrbuf, 0, sizeof( szErrbuf ) );

	rpcap->pcap = pcap_open_live( rwifi->device, 800, 1, 1, szErrbuf );
	if ( rpcap->pcap == NULL ) {
		printf( "rawwifi_init : Unable to open interface %s in pcap_out: %s\n",  rwifi->device, szErrbuf );
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

	rpcap->pcap = pcap_open_live( rwifi->device, 2048, 1, read_timeout_ms, szErrbuf );
	if ( rpcap->pcap == NULL ) {
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
			break;

		default:
			printf( "!!! unknown encapsulation on %s !\n", rwifi->device );
			return NULL;
	}

	printf( "pcap_in program : %s\n", szProgram );

	if ( pcap_compile( rpcap->pcap, &bpfprogram, szProgram, 1, 0 ) < 0 ) {
		printf( "%s\n", szProgram );
		printf( "%s\n", pcap_geterr( rpcap->pcap ) );
		return NULL;
	} else {
		if ( pcap_setfilter( rpcap->pcap, &bpfprogram ) == -1 ) {
			printf( "%s\n", szProgram );
			printf( "%s\n", pcap_geterr( rpcap->pcap ) );
		} else {
		}
		pcap_freecode( &bpfprogram );
	}

	rpcap->port = port;
	rpcap->blocking = blocking;

	return rpcap;
}


rawwifi_t* rawwifi_init( const char* device, int rx_port, int tx_port, int blocking, int read_timeout_ms )
{
	rawwifi_t* rwifi = (rawwifi_t*)malloc( sizeof(rawwifi_t) );
	memset( rwifi, 0, sizeof( rawwifi_t ) );

	rwifi->device = strdup( device );
	rwifi->iw_socket = -1;//iw_sockets_open();

	rwifi->out = setup_tx( rwifi, rx_port, blocking );
	rwifi->in = setup_rx( rwifi, tx_port, blocking, read_timeout_ms );
	rwifi->recv_timeout_ms = read_timeout_ms;

	if ( !rwifi->out || !rwifi->in ) {
		return NULL;
	}

	rawwifi_init_txbuf( rwifi->tx_buffer );
	return rwifi;
}


int32_t rawwifi_recv_quality( rawwifi_t* rwifi )
{
	if ( rwifi == 0 ) {
		return 0;
	}
	return rwifi->recv_quality;
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
