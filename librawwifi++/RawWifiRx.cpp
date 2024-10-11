#include <Debug.h>
#include "RawWifi.h"
#include "constants.h"
#include "radiotap.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define	le16_to_cpu(x) (x)
#define	le32_to_cpu(x) (x)
#else
#define	le16_to_cpu(x) ((((x)&0xff)<<8)|(((x)&0xff00)>>8))
#define	le32_to_cpu(x) \
((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)&0xff0000)>>8)|(((x)&0xff000000)>>24))
#endif
#define	unlikely(x) (x)

#ifdef __linux__
static uint64_t _rawwifi_get_tick()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}
#elif _WIN32
#endif

using namespace rawwifi;

RawWifi::PenumbraRadiotapData RawWifi::iterateIEEE80211Header( struct ieee80211_radiotap_header* payload, uint32_t payload_len )
{
	struct ieee80211_radiotap_iterator rti;
	PenumbraRadiotapData prd;
	int32_t ret;

	ret = ieee80211_radiotap_iterator_init( &rti, payload, payload_len );
	if ( ret < 0 ) {
		throw std::runtime_error( "ieee80211_radiotap_iterator_init() failed : " + std::to_string( ret ) );
	}

// 	int link_update = 0;
	int n = 0;
	while ( ( n = ieee80211_radiotap_iterator_next(&rti) ) == 0 ) {
		switch ( rti.this_arg_index ) {
			case IEEE80211_RADIOTAP_RATE: {
				// rwifi->recv_link.rate = (*rti.this_arg);
				break;
			}
			case IEEE80211_RADIOTAP_CHANNEL: {
				// uint16_t channel = le16_to_cpu(*((uint16_t *)rti.this_arg));
				uint16_t flags = le16_to_cpu(*((uint16_t *)(rti.this_arg + 2)));
// 				rwifi->recv_link.channel = channel;
				prd.nChannelFlags = flags;
				break;
			}
			case IEEE80211_RADIOTAP_ANTENNA: {
				// rwifi->recv_link.antenna = (*rti.this_arg) + 1;
				break;
			}
			case IEEE80211_RADIOTAP_FLAGS: {
				prd.nRadiotapFlags = *rti.this_arg;
				break;
			}
			case IEEE80211_RADIOTAP_DBM_ANTSIGNAL: {
				// rwifi->recv_link.signal = (int8_t)(*rti.this_arg);
				break;
			}
			default: {
				break;
			}
		}
	}

	return prd;
}


int32_t RawWifi::ReceiveFrame( uint8_t buffer[1450*2], int32_t* valid, uint32_t timeout_ms )
{
	struct pcap_pkthdr* ppcapPacketHeader = nullptr;
	uint8_t* payload = 0;

	gDebug() << "pcap_next_ex() lock";
	mRxMutex.lock();
	gDebug() << "pcap_next_ex()";
	int retval = pcap_next_ex( mRxPcap->getPcap(), &ppcapPacketHeader, (const uint8_t**)&payload );
	gDebug() << "pcap_next_ex() retval = " << retval;

	if ( retval < 0 ) {
		gError() << "pcap_next_ex() failed : " << pcap_geterr( mRxPcap->getPcap() );
		mRxMutex.unlock();
		return retval;
	}

	if ( retval != 1 ) {
		if ( mBlocking ) {
			mRxMutex.unlock();
			gError() << "pcap_next_ex() unexpected failure : " << pcap_geterr( mRxPcap->getPcap() );
			return retval;
		}
		mRxMutex.unlock();
		// Timeout
		return 0;
	}

	if ( payload == nullptr ) {
		gError() << "pcap_next_ex() returned a null payload";
		mRxMutex.unlock();
		return -1;
	}

	uint16_t u16HeaderLen = (payload[2] + (payload[3] << 8));

	if ( ppcapPacketHeader->len < ( u16HeaderLen + mRxPcap->getHeaderLength() ) ) {
		gError() << "ppcapPacketHeader->len < ( u16HeaderLen + rwifi->n80211HeaderLength )";
		mRxMutex.unlock();
		return 0;
	}

	int32_t bytes = (int32_t)ppcapPacketHeader->len - (int32_t)( u16HeaderLen + mRxPcap->getHeaderLength() );
	if ( bytes < 0 ) {
		gError() << "bytes < 0";
		mRxMutex.unlock();
		return 0;
	}

	const uint8_t* u8aIeeeHeader = payload + u16HeaderLen;
	uint8_t port = u8aIeeeHeader[sizeof(uint32_t) + sizeof(uint8_t)*6 + sizeof(uint8_t)*5];
	if ( port != mRxPort ) {
		mRxMutex.unlock();
		return 0;
	}

	PenumbraRadiotapData prd;
	try {
		prd = iterateIEEE80211Header( (struct ieee80211_radiotap_header*)payload, ppcapPacketHeader->len );
	} catch ( std::exception& e ) {
		gError() << "iterateIEEE80211Header() failed : " << e.what();
		mRxMutex.unlock();
		return 0;
	}

	*valid = -1;
	if ( prd.nRadiotapFlags & IEEE80211_RADIOTAP_F_FCS ) {
		bytes -= 4;
		*valid = ( ( prd.nRadiotapFlags & IEEE80211_RADIOTAP_F_BADFCS ) == 0 );
	}

	memcpy( buffer, payload + u16HeaderLen + mRxPcap->getHeaderLength(), bytes );

	mRxMutex.unlock();
	return bytes;
}


int32_t RawWifi::ProcessFrame( uint8_t frameBuffer[1450*2], uint32_t frameSize, uint8_t dataBuffer[1450*2], int32_t* frameValid )
{
	PacketHeader* frameHeader = (PacketHeader*)frameBuffer;
	uint8_t* pu8Payload = frameBuffer + sizeof( PacketHeader );
	frameSize -= sizeof( PacketHeader );

	if ( frameHeader->header_crc != crc16( (uint8_t*)frameHeader, sizeof(PacketHeader) - sizeof(uint16_t) ) ) {
		gError() << "Invalid header CRC, dropping ! [" << frameHeader->block_id << ":" << frameHeader->packet_id << "]";
		return 0;
	}

	if ( frameHeader->packet_id >= frameHeader->packets_count
		|| frameHeader->packet_id > RAWWIFI_MAX_PACKET_PER_BLOCK
		|| frameHeader->packets_count > RAWWIFI_MAX_PACKET_PER_BLOCK
	) {
		gError() << "Invalid packet_id/packets_count, dropping !\n"
				<< "header = {\n"
				<< "    block_id = " << (int)frameHeader->block_id << "\n"
				<< "    packet_id = " << (int)frameHeader->packet_id << "\n"
				<< "    packets_count = " << (int)frameHeader->packets_count << "\n"
				<< "    retry_id = " << (int)frameHeader->retry_id << "\n"
				<< "    retries_count = " << (int)frameHeader->retries_count << "\n"
				<< "    block_flags = " << (int)frameHeader->block_flags << "\n"
				<< "}\n";
		return 0;
	}
	/*
	if ( rwifi->recv_last_returned >= header->block_id + 32 ) {
		// More than 32 underruns, TX has probably been resetted
		rwifi->recv_last_returned = 0;
		free( rwifi->recv_block );
		rwifi->recv_block = NULL;
	}
	*/

	int32_t isValid = 0;
	if ( *frameValid == 0 ) {
		gTrace() << "pcap says frame's CRC is invalid";
	} else if ( *frameValid == 1 ) {
		gTrace() << "pcap says frame's CRC is valid";
		isValid = 1;
	} else if ( (int32_t)*frameValid < 0 ) { // pcap doesn't know if frame is valid, so check it
		uint32_t calculated_crc = crc32( pu8Payload, frameSize );
		isValid = ( frameHeader->crc == calculated_crc );
		if ( isValid == 0 ) {
			gWarning() << "Invalid CRC ! (" << std::hex << frameHeader->crc << " != " << calculated_crc << ")";
		}
	}

	if ( frameHeader->block_id <= mRxLastCompletedBlockId ) {
		gTrace() << "Block " << frameHeader->block_id << " already completed";
		return 0;
	}

	gTrace() << "header = {\n"
			<< "    block_id = " << (int)frameHeader->block_id << "\n"
			<< "    packet_id = " << (int)frameHeader->packet_id << "\n"
			<< "    packets_count = " << (int)frameHeader->packets_count << "\n"
			<< "    retry_id = " << (int)frameHeader->retry_id << "\n"
			<< "    retries_count = " << (int)frameHeader->retries_count << "\n"
			<< "    block_flags = " << (int)frameHeader->block_flags << "\n"
			<< "}, valid = " << isValid;

	memcpy( dataBuffer, pu8Payload, frameSize );
	*frameValid = isValid;
	return frameSize;
}


int32_t RawWifi::Receive( uint8_t* buffer, uint32_t maxBufferSize, bool* valid, uint32_t timeout_ms )
{
	uint64_t startTickMicros = _rawwifi_get_tick();
	bool packetComplete = false;

	while ( ( timeout_ms == 0 and mBlocking ) or ( _rawwifi_get_tick() - startTickMicros < timeout_ms * 1000 and not packetComplete ) ) {
		uint8_t frameBuffer[1450*2] = { 0 };
		int32_t frameValid = 0;
		int32_t frameSize = ReceiveFrame( frameBuffer, &frameValid, timeout_ms );

		if ( frameSize < 0 ) {
			gError() << "ReceiveFrame() failed : " << frameSize;
			return frameSize;
		}
		if ( frameSize == 0 ) {
			continue;
		}
		if ( frameSize > RAWWIFI_MAX_USER_PACKET_LENGTH + 128 ) {
			gError() << "frameSize > RAWWIFI_MAX_USER_PACKET_LENGTH + 128";
			continue;
		}

		PacketHeader* frameHeader = (PacketHeader*)frameBuffer;
		uint8_t dataBuffer[1450*2] = { 0 };
		int32_t dataSize = ProcessFrame( frameBuffer, frameSize, dataBuffer, &frameValid );

		if ( dataSize < 0 ) {
			gError() << "ProcessFrame() failed : " << dataSize;
			return dataSize;
		}
		if ( dataSize == 0 ) {
			continue;
		}

		// Simple case : only one packet in the block
		if ( frameHeader->packets_count == 1 ) {
			if ( frameValid ) {
				memcpy( buffer, dataBuffer, dataSize );
				return dataSize;
			}
			continue;
		}

		// New block
		if ( frameHeader->block_id != mRxBlock.id ) {
			mRxBlock.id = frameHeader->block_id;
			mRxBlock.valid = 0;
			mRxBlock.ticks = _rawwifi_get_tick();
			mRxBlock.packets_count = frameHeader->packets_count;
			memset( mRxBlock.packets, 0, sizeof( mRxBlock.packets ) );
		}

		// Check if the packet is already valid, if not, store it
		if ( mRxBlock.packets[frameHeader->packet_id].size == 0 or not mRxBlock.packets[frameHeader->packet_id].valid ) {
			memcpy( mRxBlock.packets[frameHeader->packet_id].data, dataBuffer, dataSize );
			mRxBlock.packets[frameHeader->packet_id].size = dataSize;
			mRxBlock.packets[frameHeader->packet_id].valid = frameValid;
			gTrace() << "set packet " << frameHeader->packet_id << "/" << frameHeader->packets_count << " data to " << (void*)mRxBlock.packets[frameHeader->packet_id].data;
		} else {
			gTrace() << "packet " << frameHeader->packet_id << "/" << frameHeader->packets_count << " already valid";
		}

		// Check if the block is complete
		bool blockOk = true;
		for ( uint32_t i = 0; i < mRxBlock.packets_count; i++ ) {
			if ( mRxBlock.packets[i].size == 0 or not mRxBlock.packets[i].valid ) {
				blockOk = false;
				break;
			}
		}

		// All packets received, return the block as is even if it's not valid
		if ( !blockOk and ( frameHeader->packet_id >= mRxBlock.packets_count - 1 or mRxBlock.packets[mRxBlock.packets_count-1].size > 0 ) ) {
			gDebug() << "Block " << mRxBlock.id << " half empty, deal with it";
			blockOk = true;
		}

		if ( blockOk ) {
			uint32_t allValid = 0;
			uint32_t offset = 0;
			for ( uint32_t i = 0; i < mRxBlock.packets_count; i++ ) {
				if ( mRxBlock.packets[i].size > 0 ) {
					if ( offset + mRxBlock.packets[i].size >= maxBufferSize - 1 ) {
						break;
					}
					gTrace() << "[" << i+1 << "/" << mRxBlock.packets_count
							<< "] memcpy( " << (void*)(buffer + offset)
							<< ", " << (void*)mRxBlock.packets[i].data
							<< ", " << mRxBlock.packets[i].size
							<< " ) [" << ( mRxBlock.packets[i].valid ? "valid" : "invalid" ) << "]";
					memcpy( buffer + offset, mRxBlock.packets[i].data, mRxBlock.packets[i].size );
					offset += mRxBlock.packets[i].size;
					allValid += ( mRxBlock.packets[i].valid != 0 );
				} else {
					gDebug() << "Leak : packet " << i << "/" << mRxBlock.packets_count << " is missing, lost size is " << mRxBlock.packets[i].size << " bytes, filling with zeros";
					// if ( i < mRxBlock.packets_count - 1 && rwifi->recv_recover == RAWWIFI_FILL_WITH_ZEROS ) {
						memset( buffer + offset, 0, RAWWIFI_MAX_USER_PACKET_LENGTH - mHeadersLenght );
						offset += RAWWIFI_MAX_USER_PACKET_LENGTH - mHeadersLenght;
					// }
				}
				if ( offset >= maxBufferSize - 1 ) {
					break;
				}
			}
			mRxBlock.valid = ( allValid == mRxBlock.packets_count );
			/*
			if ( mRxBlock.valid ) {
				rwifi->recv_perf_valid++;
			} else {
				rwifi->recv_perf_invalid++;
			}
			*/
			*valid = mRxBlock.valid;
			gTrace() << "block " << mRxBlock.id << " " << ( mRxBlock.valid ? "valid":"invalid" ) << " : " << offset << " bytes total";
			mRxLastCompletedBlockId = mRxBlock.id;
			/*
			if ( frameHeader->block_flags & RAWWIFI_BLOCK_FLAGS_HAMMING84 ) {
				return rawwifi_hamming84_decode( pret, pret, offset );
			}
			*/
			return offset;
		}
	}

	return 0;
}
