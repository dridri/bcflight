#include "PcapHandler.h"

#include <stdexcept>

#include <Debug.h>

using namespace rawwifi;

PcapHandler::PcapHandler( const std::string& device, uint8_t port, bool blocking, int32_t read_timeout_ms )
	: mPcap( nullptr )
	, mDevice( device )
	, mPort( port )
	, mBlocking( blocking )
	, mHeaderLength( 0 )
{
	fDebug( device, (int)port, (int)blocking, read_timeout_ms );

	char errbuf[PCAP_ERRBUF_SIZE];

	mPcap = pcap_create( device.c_str(), errbuf );
	if ( !mPcap ) {
		throw std::runtime_error( std::string( "Failed to create pcap handler: " ) + errbuf );
	}

	if ( pcap_set_snaplen( mPcap, 4096 ) ) {
		throw std::runtime_error( "Failed to set snaplen" );
	}

	if ( pcap_set_promisc( mPcap, 1 ) ) {
		throw std::runtime_error( "Failed to set promisc" );
	}

	if ( read_timeout_ms >= 0 and pcap_set_timeout( mPcap, read_timeout_ms ) ) {
		throw std::runtime_error( "Failed to set timeout" );
	}

	if ( pcap_set_immediate_mode( mPcap, 1 ) ) {
		throw std::runtime_error( "Failed to set immediate mode" );
	}

	// TBD
	// pcap_set_buffer_size
	// pcap_set_tstamp_type
	// pcap_set_rfmon
	// pcap_setdirection

	if ( pcap_setnonblock( mPcap, !blocking, errbuf ) ) {
		throw std::runtime_error( std::string( "Failed to set blocking: " ) + errbuf );
	}

	if ( pcap_activate( mPcap ) ) {
		throw std::runtime_error( "Failed to activate pcap handler" );
	}
}


PcapHandler::~PcapHandler()
{
	if ( mPcap ) {
		pcap_close( mPcap );
	}
}


void PcapHandler::CompileFilter()
{
	gDebug() << "Compiling PCAP filter for port " << (int)mPort << " on device " << mDevice;
	char program[512] = "";
	struct bpf_program bpfprogram;

	switch ( pcap_datalink( mPcap ) ) {
		case DLT_PRISM_HEADER:
			gDebug() << "DLT_PRISM_HEADER Encap";
			mHeaderLength = 0x20; // ieee80211 comes after this
			sprintf( program, "radio[0x4a:4]==0x13223344 && radio[0x4e:2] == 0x55%.2x", mPort );
			break;
		case DLT_IEEE802_11_RADIO:
			gDebug() << "DLT_IEEE802_11_RADIO Encap";
			mHeaderLength = 0x18; // ieee80211 comes after this
			sprintf( program, "ether[0x0a:4]==0x13223344 && ether[0x0e:2] == 0x55%.2x", mPort );
			break;
		default:
			throw std::runtime_error( "Unknown encapsulation on " + mDevice );
	}

	// return;

	uint32_t i = 0;
	uint32_t retries = 8;
	bool ok = false;
	for ( i = 0; i < retries and not ok; i++ ) {
		if ( pcap_compile( mPcap, &bpfprogram, program, 1, 0 ) >= 0 ) {
			ok = true;
		}
	}
	if ( !ok ) {
		throw std::runtime_error( std::string( "Failed to compile PCAP filter: " ) + pcap_geterr( mPcap ) );
	}
	ok = false;
	for ( i = 0; i < retries and not ok; i++ ) {
		if ( pcap_setfilter( mPcap, &bpfprogram ) >= 0 ) {
			ok = true;
		}
	}
	if ( !ok ) {
		throw std::runtime_error( std::string( "Failed to set PCAP filter: " ) + pcap_geterr( mPcap ) );
	}
	pcap_freecode( &bpfprogram );
}
