/*
 * BCFlight
 * Copyright (C) 2017 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef NRF24L01_H
#define NRF24L01_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <mutex>
#include <functional>
#include <atomic>
#include <condition_variable>

#include "Link.h"
#include "RF24/RF24.h"

/*
	Channels :
	0 => 2400 Mhz (RF24 channel 1)
	1 => 2401 Mhz (RF24 channel 2)
	76 => 2476 Mhz (RF24 channel 77) standard
	83 => 2483 Mhz (RF24 channel 84)
	124 => 2524 Mhz (RF24 channel 125)
	125 => 2525 Mhz (RF24 channel 126)
*/

class Main;
class Config;

class nRF24L01 : public Link
{
public:
	nRF24L01( const std::string& device, uint8_t cspin, uint8_t cepin, uint8_t channel = 100, uint32_t input_port = 0, uint32_t output_port = 1, bool drop_invalid_packets = false );
	~nRF24L01();

	int Connect();
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

	static int flight_register( Main* main );

protected:
	void Interrupt();
	bool ThreadRun();
	void PerfUpdate();

	typedef struct __attribute__((packed)) {
		uint8_t block_id;
		uint8_t packet_id;
		uint8_t packets_count;
		uint8_t packet_size;
	} Header;

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

	static Link* Instanciate( Config* config, const std::string& lua_object );
	int Send( const void* buf, uint32_t len, bool ack );
	int Receive( void* buf, uint32_t len );

	std::string mDevice;
	uint8_t mCSPin;
	uint8_t mCEPin;
	nRF24::RF24* mRadio;
	bool mBlocking;
	bool mDropBroken;
	uint8_t mChannel;
	uint32_t mInputPort;
	uint32_t mOutputPort;
	uint32_t mRetries;
	uint32_t mReadTimeout;
	std::mutex mInterruptMutex;

	// TX
	uint8_t mTXBlockID;

	// RX
	Block mRxBlock;
	std::list<std::pair<uint8_t*, uint32_t>> mRxQueue;
	std::mutex mRxQueueMutex;

	// Perfs
	std::mutex mPerfMutex;
	bool mRPD;
	float mSmoothRPD;
	int32_t mRxQuality;
	uint64_t mPerfTicks;
	uint32_t mPerfLastRxBlock;
	uint32_t mPerfValidBlocks;
	uint32_t mPerfInvalidBlocks;
};

#endif // NRF24L01_H
