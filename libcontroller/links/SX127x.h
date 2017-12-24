#ifndef SX127X_H
#define SX127X_H

#include <atomic>
#include <mutex>
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

	typedef struct {
		std::string device;
		int32_t resetPin;
		int32_t txPin;
		int32_t rxPin;
		int32_t irqPin;
		bool blocking;
		bool dropBroken;
		Modem modem;
		uint32_t frequency;
		int32_t inputPort;
		int32_t outputPort;
		int32_t retries;
		int32_t readTimeout;
		int32_t bitrate;
		int32_t bandwidth;
		int32_t fdev;
	} Config;

	SX127x(const SX127x::Config& config);
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

	int Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 );
	int Read( void* buf, uint32_t len, int32_t timeout );
	int32_t WriteAck( const void* buf, uint32_t len );

protected:
	typedef struct __attribute__((packed)) {
		uint8_t block_id;
		uint8_t packet_id;
		uint8_t packets_count;
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

	void SetupRX( const RxConfig_t& conf );
	void SetupTX( const TxConfig_t& conf );
	void Interrupt();
	void reset();
	bool ping();
	void startReceiving();
	void startTransmitting();
	void setModem( Modem modem );
	uint32_t getOpMode();
	bool setOpMode( uint32_t mode );
	uint8_t readRegister( uint8_t reg );
	bool writeRegister( uint8_t reg, uint8_t value );

	SPI* mSPI;

	std::string mDevice;
	int32_t mResetPin;
	int32_t mTXPin;
	int32_t mRXPin;
	int32_t mIRQPin;
	bool mBlocking;
	bool mDropBroken;
	Modem mModem;
	uint32_t mFrequency;
	int32_t mInputPort;
	int32_t mOutputPort;
	int32_t mRetries;
	int32_t mReadTimeout;
	uint32_t mBitrate;
	uint32_t mBandwidth;
	uint32_t mFdev;
	std::atomic_bool mSending;
	std::atomic_bool mSendingEnd;
	uint64_t mSendTime;

	void PerfUpdate();
	std::mutex mPerfMutex;
	int32_t mRSSI;
	int32_t mRxQuality;
	int32_t mPerfTicks;
	int32_t mPerfLastRxBlock;
	int32_t mPerfValidBlocks;
	int32_t mPerfInvalidBlocks;

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
	std::list<std::pair<uint8_t*, uint32_t>> mRxQueue;
	std::mutex mRxQueueMutex;
};

#endif // SX127X_H
