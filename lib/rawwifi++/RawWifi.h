#pragma once

#include <stdint.h>
#include <pcap.h>

#include <string>
#include <mutex>

#include "PcapHandler.h"
#include "ieee80211_radiotap.h"

#define RAWWIFI_MAX_USER_PACKET_LENGTH 1450 // same as MTU
#define RAWWIFI_MAX_PACKET_PER_BLOCK 128

namespace rawwifi {

class RawWifi
{
public:
	typedef enum {
		RX_FAST = 0,
		RX_FEC_WEIGHTED = 1,
		RX_FEC_CEC = 2
	} RxFecMode;

	typedef enum {
		FILL_WITH_ZEROS = 0, // default : fill missing parts of blocks with zeros
		CONTIGUOUS = 1,  // put all the packets one behind the other
	} RxBlockRecoverMode;

	typedef enum {
		BLOCK_FLAGS_NONE = 0,
		BLOCK_FLAGS_HAMMING84 = 1,
		BLOCK_FLAGS_EXTRA_HEADER_ROOM = 2,
	} BlockFlags;

	RawWifi( const std::string& device, uint8_t rx_port, uint8_t tx_port, bool blocking, int32_t read_timeout_ms );
	~RawWifi();

	int32_t Send( const uint8_t* buffer, uint32_t size, uint8_t retries = 1 );
	int32_t Receive( uint8_t* buffer, uint32_t size, bool* valid, uint32_t timeout_ms = 0 );

protected:
	typedef struct RadiotapHeader {
		uint8_t version;
		uint8_t padding;
		uint16_t length;
		uint32_t presence;
		// uint8_t rate;
		// uint8_t padding2;
		uint16_t txFlags;
		struct {
			uint8_t known;
			uint8_t flags;
			uint8_t modulationIndex;
		} __attribute__ ((packed)) mcs;
	} __attribute__ ((packed)) RadiotapHeader;

	typedef struct PacketHeader {
		uint32_t block_id;
		uint8_t packet_id;
		uint8_t packets_count;
		uint8_t retry_id:4;
		uint8_t _align0:4;
		uint8_t retries_count:4;
		uint8_t block_flags:4;
		uint32_t crc;
		uint16_t header_crc; // Whole frame must be discarded if the header is corrupted
	} __attribute__ ((packed)) PacketHeader;

	typedef struct PenumbraRadiotapData {
		int nChannel;
		int nChannelFlags;
		int nRate;
		int nAntenna;
		int nRadiotapFlags;
	} __attribute__((packed)) PenumbraRadiotapData;

	typedef struct Packet {
		uint8_t data[2048];
		uint32_t size;
		uint32_t valid;
	} Packet;

	typedef struct Block {
		uint32_t id;
		uint64_t ticks;
		uint16_t valid;
		uint16_t packets_count;
		Packet packets[RAWWIFI_MAX_PACKET_PER_BLOCK];
	} Block;

	void initTxBuffer( uint8_t* buffer, uint32_t size );
	int32_t SendFrame( const uint8_t* data, uint32_t datalen, uint32_t block_id, BlockFlags block_flags, uint16_t packet_id, uint16_t packets_count, uint32_t retries );

	int32_t ReceiveFrame( uint8_t buffer[1450*2], int32_t* valid, uint32_t read_timeout_ms = 0 );
	int32_t ProcessFrame( uint8_t frameBuffer[1450*2], uint32_t frameSize, uint8_t dataBuffer[1450*2], int32_t* frameValid );
	PenumbraRadiotapData iterateIEEE80211Header( struct ieee80211_radiotap_header* payload, uint32_t payload_len );

	uint32_t crc32( const uint8_t* buf, uint32_t len );
	uint16_t crc16( const uint8_t* buf, uint32_t len );

	// Parameters
	std::string mDevice;
	uint8_t mRxPort;
	uint8_t mTxPort;
	bool mBlocking;
	int32_t mReadTimeoutMs;
	// Global state
	std::mutex mCompileMutex;
	int32_t mIwSocket;
	uint32_t mHeadersLenght;
	// RX
	PcapHandler* mRxPcap;
	RxFecMode mRxFecMode;
	RxBlockRecoverMode mRxBlockRecoverMode;
	std::mutex mRxMutex;
	uint32_t mRxLastCompletedBlockId;
	Block mRxBlock;
	// TX
	PcapHandler* mTxPcap;
	uint8_t mTxBuffer[4096];
	uint32_t mSendBlockId;
	std::mutex mTxMutex;
};

} // namespace rawwifi
