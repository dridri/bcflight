#ifndef SX127X_H
#define SX127X_H

#include <atomic>
#include <Mutex.h>
#include <list>
#include "Link.h"

class Main;
class SPI;

class SX127x : public Link
{
public:
	typedef enum {
		FSK,
		LoRa,
	} Modem;

	SX127x( Config* config, const string& lua_object );
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

	static int flight_register( Main* main );

protected:
	static Link* Instanciate( Config* config, const string& lua_object );

	typedef struct __attribute__((packed)) {
		uint8_t block_id;
		uint8_t packet_id;
		uint8_t packets_count;
		uint8_t crc;
	} Header;

	typedef struct RxConfig_t
	{
		uint32_t bandwidth;
		uint32_t datarate;
		uint8_t coderate;
		uint32_t bandwidthAfc;
		uint16_t preambleLen;
		uint16_t symbTimeout;
		uint8_t payloadLen;
		bool crcOn;
		bool freqHopOn;
		uint8_t hopPeriod;
		bool iqInverted;
	} RxConfig_t;

	typedef struct TxConfig_t
	{
		uint32_t fdev;
		uint32_t bandwidth;
		uint32_t datarate;
		uint8_t coderate;
		uint16_t preambleLen;
		bool crcOn;
		bool freqHopOn;
		uint8_t hopPeriod;
		bool iqInverted;
		uint32_t timeout;
	} TxConfig_t;

	int32_t Setup( SPI* spi );
	void SetupRX( SPI* spi, const RxConfig_t& conf );
	void SetupTX( SPI* spi, const TxConfig_t& conf );
	void Interrupt( SPI* spi );
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

	string mDevice;
	int32_t mResetPin;
	int32_t mTXPin;
	int32_t mRXPin;
	int32_t mIRQPin;
	bool mBlocking;
	bool mDropBroken;
	bool mEnableTCXO;
	Modem mModem;
	uint32_t mFrequency;
	int32_t mInputPort;
	int32_t mOutputPort;
	int32_t mRetries;
	int32_t mReadTimeout;
	uint32_t mBitrate;
	uint32_t mBandwidth;
	uint32_t mBandwidthAfc;
	uint32_t mFdev;
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
	list< uint64_t > mPerfHistory; // [ticks]ValidBlocks

	typedef struct {
		SPI* spi;
		string device;
		int32_t resetPin;
		int32_t irqPin;
	} Diversity;
	Diversity* mDiversity;
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
