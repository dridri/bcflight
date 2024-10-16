#ifndef SX127X_H
#define SX127X_H

#include <atomic>
#include <Mutex.h>
#include <Lua.h>
#include <list>
#include "Link.h"

class SPI;

LUA_CLASS class SX127x : public Link
{
public:
	LUA_EXPORT typedef enum {
		FSK,
		LoRa,
	} Modem;

	LUA_EXPORT SX127x();
	~SX127x();

	int Connect();
	void setFrequency( float f );
	int setBlocking( bool blocking );
	void setRetriesCount( int retries );
	int retriesCount() const;
	int32_t Channel();
	int32_t Frequency();
	int32_t RxQuality();
	int32_t RxLevel();
	uint32_t fullReadSpeed();

	SyncReturn Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 );
	SyncReturn Read( void* buf, uint32_t len, int32_t timeout );
	SyncReturn WriteAck( const void* buf, uint32_t len );

	virtual string name() const;
	virtual LuaValue infos() const;

protected:
	void init();

	typedef struct __attribute__((packed)) {
		uint8_t small_packet : 1;
		uint8_t block_id : 7;
		uint8_t packet_id;
		uint8_t packets_count;
		uint8_t crc;
	} Header;

	typedef struct __attribute__((packed)) {
		uint8_t small_packet : 1;
		uint8_t block_id : 7;
		uint8_t crc;
	} HeaderMini;

	typedef struct TxRxConfig_t
	{
		// Common parameters for both TX and RX
		uint32_t bandwidth;       // Bandwidth of the signal (FSK/LoRa)
		uint32_t datarate;        // Data rate of the signal (bps)
		uint8_t coderate;         // Coding rate for LoRa (FSK doesn't use this)
		uint16_t preambleLen;     // Length of the preamble (in symbols)
		bool crcOn;               // Enable CRC checking
		bool freqHopOn;           // Enable frequency hopping
		uint8_t hopPeriod;        // Hopping period (0 = disabled)
		bool iqInverted;          // Inverts the IQ signal

		// TX-specific parameters
		uint32_t fdev;            // Frequency deviation for FSK modulation
		uint32_t timeout;         // TX timeout duration (ms)

		// RX-specific parameters
		uint32_t bandwidthAfc;    // AFC bandwidth (FSK only)
		uint16_t symbTimeout;     // Symbol timeout for RX
		uint8_t payloadLen;       // Payload length for RX (LoRa implicit mode only)
	} TxRxConfig_t;

	int32_t Setup( SPI* spi );
	void SetupModem( SPI* spi, const TxRxConfig_t& conf );
	void Interrupt( SPI* spi, int32_t ledPin = -1 );
	void reset();
	bool ping( SPI* spi = nullptr );
	void setFrequency( SPI* spi, float f );
	void startReceiving( SPI* spi = nullptr );
	void startTransmitting();
	void setModem( SPI* spi, Modem modem );
	uint32_t getOpMode( SPI* spi );
	bool setOpMode( SPI* spi, uint32_t mode );
	uint8_t readRegister( uint8_t reg );
	bool writeRegister( uint8_t reg, uint8_t value );
	uint8_t readRegister( SPI* spi, uint8_t reg );
	bool writeRegister( SPI* spi, uint8_t reg, uint8_t value );

	SPI* mSPI;
	bool mReady;
	LUA_PROPERTY("device") string mDevice;
	LUA_PROPERTY("resetpin") int32_t mResetPin;
	LUA_PROPERTY("txpin") int32_t mTXPin;
	LUA_PROPERTY("rxpin") int32_t mRXPin;
	LUA_PROPERTY("irqpin") int32_t mIRQPin;
	LUA_PROPERTY("ledpin") int32_t mLedPin;
	LUA_PROPERTY("blocking") bool mBlocking;
	LUA_PROPERTY("drop") bool mDropBroken;
	bool mEnableTCXO;
	LUA_PROPERTY("modem") SX127x::Modem mModem;
	LUA_PROPERTY("frequency") uint32_t mFrequency;
	int32_t mInputPort;
	int32_t mOutputPort;
	int32_t mRetries;
	LUA_PROPERTY("read_timeout") int32_t mReadTimeout;
	LUA_PROPERTY("bitrate") uint32_t mBitrate;
	LUA_PROPERTY("bandwidth") uint32_t mBandwidth;
	LUA_PROPERTY("bandwidthAfc") uint32_t mBandwidthAfc;
	LUA_PROPERTY("fdev") uint32_t mFdev;
	atomic_bool mSending;
	atomic_bool mSendingEnd;
	uint64_t mSendTime;

	void PerfUpdate();
	Mutex mPerfMutex;
	int32_t mRSSI;
	int32_t mRxQuality;
	int32_t mPerfTicks;
	int32_t mPerfLastRxBlock;
	int32_t mPerfValidBlocks;
	int32_t mPerfInvalidBlocks;
	int32_t mPerfBlocksPerSecond;
	int32_t mPerfMaxBlocksPerSecond;
	list< uint64_t > mTotalHistory; // [ticks]
	list< uint64_t > mMissedHistory; // [ticks]MissedBlocks
	list< uint64_t > mPerfHistory; // [ticks]ValidBlocks
	int32_t mPerfTotalBlocks;
	int32_t mPerfMissedBlocks;

	SPI* mDiversitySpi;
	LUA_PROPERTY("diversity.device") string mDiversityDevice;
	LUA_PROPERTY("diversity.resetpin") int32_t mDiversityResetPin;
	LUA_PROPERTY("diversity.irqpin") int32_t mDiversityIrqPin;
	LUA_PROPERTY("diversity.ledpin") int32_t mDiversityLedPin;
	Mutex mInterruptMutex;

	// TX
	uint8_t mTXBlockID;

	// RX
	int Receive( uint8_t* data, uint32_t datalen, void* pRet, uint32_t len );
	typedef struct {
		uint8_t data[32];
		uint8_t size;
		bool valid;
	} Packet;
	typedef struct {
		uint8_t block_id;
		uint8_t packets_count;
		Packet packets[256];
		bool received;
	} Block;
	Block mRxBlock;
	list<pair<uint8_t*, uint32_t>> mRxQueue;
	Mutex mRxQueueMutex;
};

#endif // SX127X_H
