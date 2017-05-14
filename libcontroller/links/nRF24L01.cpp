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

#include <cmath>
#include <iostream>
#include <string.h>

#ifdef BOARD_rpi
#include <wiringPi.h>
#include <GPIO.h>
#endif

#include "nRF24L01.h"
#include <Thread.h>

#define CONTINUE -2

nRF24L01::nRF24L01( const std::string& device, uint8_t cspin, uint8_t cepin, int8_t irqpin, uint8_t channel, uint32_t input_port, uint32_t output_port, bool drop_invalid_packets )
	: Link()
	, mCSPin( cspin )
	, mCEPin( cepin )
	, mIRQPin( irqpin )
	, mRadio( nullptr )
	, mBlocking( true )
	, mDropBroken( drop_invalid_packets )
	, mChannel( std::max( 1u, std::min( 126u, (uint32_t)channel ) ) )
	, mInputPort( input_port )
	, mOutputPort( output_port )
	, mRetries( 1 )
	, mReadTimeout( 2000 )
	, mTXBlockID( 0 )
	, mRPD( false )
	, mSmoothRPD( 0 )
	, mRxQuality( 0 )
	, mPerfTicks( 0 )
	, mPerfLastRxBlock( 0 )
	, mPerfValidBlocks( 0 )
	, mPerfInvalidBlocks( 0 )
{
	memset( &mRxBlock, 0, sizeof(mRxBlock) );
}


nRF24L01::~nRF24L01()
{
}


int nRF24L01::Connect()
{
	if ( mRadio != nullptr ) {
		return 0;
	}

	mRadio = new nRF24::RF24( mCEPin, mCSPin, BCM2835_SPI_SPEED_8MHZ );
#ifdef BOARD_rpi
	wiringPiSetupGpio();
	mRadio->setGPIOFunctions( [](int pin, int value){ digitalWrite( pin, value ); }, [](int pin, int mode){ pinMode( pin, mode ); } );
	if ( mIRQPin >= 0 ) {
		GPIO::SetupInterrupt( mIRQPin, GPIO::Falling, [this](){ this->Interrupt(); } );
	}
#endif
	mRadio->begin();
	mRadio->setPALevel( nRF24::RF24_PA_MAX );
	mRadio->setAutoAck( true );
	mRadio->setRetries( 5, 3 );
	mRadio->setAddressWidth( 3 );
	mRadio->setPayloadSize( 32 );
	mRadio->enableAckPayload();
	mRadio->enableDynamicAck();
	mRadio->setChannel( mChannel - 1 );
	mRadio->setDataRate( nRF24::RF24_250KBPS );
	mRadio->setCRCLength( mDropBroken ? nRF24::RF24_CRC_16 : nRF24::RF24_CRC_DISABLED );
	mRadio->openWritingPipe( (uint64_t)( 0x00BCF000 | mOutputPort ) );
	mRadio->openReadingPipe( 1, (uint64_t)( 0x00BCF000 | mInputPort ) );
	mRadio->maskIRQ( true, true, false );
	mRadio->printDetails();
	mRadio->startListening();

	mConnected = true;
	return 0;
}


int nRF24L01::setBlocking( bool blocking )
{
	mBlocking = blocking;
	return 0;
}


void nRF24L01::setRetriesCount( int retries )
{
	mRetries = retries;
}


int nRF24L01::retriesCount() const
{
	return mRetries;
}


int32_t nRF24L01::Channel()
{
	return mChannel;
}


int32_t nRF24L01::Frequency()
{
	return 2400 + mChannel;
}


int32_t nRF24L01::RxQuality()
{
	return mRxQuality;
}


int32_t nRF24L01::RxLevel()
{
	return mSmoothRPD;
}


uint32_t nRF24L01::fullReadSpeed()
{
	return 0;
}


void nRF24L01::PerfUpdate()
{
	mPerfMutex.lock();
	if ( Thread::GetTick() - mPerfTicks >= 1000 ) {
		int32_t count = mRxBlock.block_id - mPerfLastRxBlock;
		if ( count == 0 or count == 1 ) {
			mRxQuality = 0;
		} else {
			if ( count < 0 ) {
				count = 255 + count;
			}
			mRxQuality = 100 * ( mPerfValidBlocks * 4 + mPerfInvalidBlocks ) / ( count * 4 );
			if ( mRxQuality > 100 ) {
				mRxQuality = 100;
			}
		}
		mPerfTicks = Thread::GetTick();
		mPerfLastRxBlock = mRxBlock.block_id;
		mPerfValidBlocks = 0;
		mPerfInvalidBlocks = 0;
	}
	mPerfMutex.unlock();
}


int nRF24L01::Read( void* pRet, uint32_t len, int32_t timeout )
{
	bool timedout = false;
	uint64_t started_waiting_at = Thread::GetTick();
	if ( timeout == 0 ) {
		timeout = mReadTimeout;
	}

	do {
/*
		mInterruptMutex.lock();
		bool rpd = mRadio->testRPD();
		if ( mRadio->available() ) {
			mRPD = rpd;
			mSmoothRPD = mSmoothRPD * 0.95f + ( mRPD ? -25 : -80 ) * 0.05f;
		}
		mInterruptMutex.unlock();
*/
		bool ok = false;
		mRxQueueMutex.lock();
		ok = ( mRxQueue.size() > 0 );
		mRxQueueMutex.unlock();
		if ( ok ) {
			break;
		}
		if ( timeout > 0 and Thread::GetTick() - started_waiting_at > (uint32_t)timeout ) {
			timedout = true;
			break;
		}

		PerfUpdate();
		usleep( 100 );
	} while ( mBlocking );
	PerfUpdate();

	mRxQueueMutex.lock();
	if ( mRxQueue.size() == 0 ) {
		mRxQueueMutex.unlock();
		if ( timedout ) {
// 			std::cout << "WARNING : Read timeout\n";
			return LINK_ERROR_TIMEOUT;
		}
		return 0;
	}

	std::pair< uint8_t*, uint32_t > data = mRxQueue.front();
	mRxQueue.pop_front();
	mRxQueueMutex.unlock();
	memcpy( pRet, data.first, data.second );
	delete data.first;
	return data.second;
}


int nRF24L01::Write( const void* data, uint32_t len, bool ack, int32_t timeout )
{
	return Send( data, len, ack );
}


int nRF24L01::Send( const void* data, uint32_t len, bool ack )
{
	mInterruptMutex.lock();
	mRadio->stopListening();

	uint8_t buf[32];
	memset( buf, 0, sizeof(buf) );
	Header* header = (Header*)buf;

	header->block_id = ++mTXBlockID;
	header->packets_count = (uint8_t)std::ceil( (float)len / (float)( 32 - sizeof(Header) ) );

	static uint32_t send_id = 0;

	uint32_t offset = 0;
	for ( uint8_t packet = 0; packet < header->packets_count; packet++ ) {
		uint32_t plen = 32 - sizeof(Header);
		if ( offset + plen > len ) {
			plen = len - offset;
		}

		memset( buf + sizeof(Header), 0, sizeof(buf) - sizeof(Header) );
		memcpy( buf + sizeof(Header), (uint8_t*)data + offset, plen );
		header->packet_size = plen;

		mRadio->writeFast( buf, sizeof(buf), not ack );
		if ( send_id++ == 2 or packet + 1 == header->packets_count ) {
			send_id = 0;
			mRadio->txStandBy();
		}

		header->packet_id++;
		offset += plen;
	}

	mRadio->startListening();
	mInterruptMutex.unlock();
	return len;
}


int nRF24L01::Receive( void* pRet, uint32_t len )
{
	uint8_t buf[32];
	Header* header = (Header*)buf;
	uint8_t* data = buf + sizeof(Header);
	int final_size = 0;

	PerfUpdate();

	memset( buf, 0, sizeof(buf) );
	mInterruptMutex.lock();
	mRadio->read( buf, sizeof(buf) );
	mInterruptMutex.unlock();
	uint32_t datalen = header->packet_size;

	if ( header->block_id == mRxBlock.block_id and mRxBlock.received ) {
		return CONTINUE;
	}

	if ( header->block_id != mRxBlock.block_id ) {
		memset( &mRxBlock, 0, sizeof(mRxBlock) );
		mRxBlock.block_id = header->block_id;
		mRxBlock.packets_count = header->packets_count;
	}

	if ( header->packets_count == 1 ) {
// 		printf( "small block\n" );
		mRxBlock.block_id = header->block_id;
		mRxBlock.received = true;
		mPerfValidBlocks++;
		memcpy( pRet, data, datalen );
		return datalen;
	}
/*
	printf( "header = {\n"
			"    block_id = %d\n"
			"    packet_id = %d\n"
			"    packets_count = %d\n", header->block_id, header->packet_id, header->packets_count );
*/
	memcpy( mRxBlock.packets[header->packet_id].data, data, datalen );
	mRxBlock.packets[header->packet_id].size = datalen;

	if ( header->packet_id >= mRxBlock.packets_count - 1 ) {
		bool valid = true;
		for ( uint8_t i = 0; i < mRxBlock.packets_count; i++ ) {
			if ( mRxBlock.packets[i].size == 0 ) {
				mPerfInvalidBlocks++;
				valid = false;
				break;
			}
			if ( final_size + mRxBlock.packets[i].size >= (int)len ) {
				mPerfInvalidBlocks++;
				valid = false;
				break;
			}
			memcpy( &((uint8_t*)pRet)[final_size], mRxBlock.packets[i].data, mRxBlock.packets[i].size );
			final_size += mRxBlock.packets[i].size;
		}
		if ( mDropBroken and valid == false ) {
			mPerfInvalidBlocks++;
			return CONTINUE;
		}
		mRxBlock.received = true;
		mPerfValidBlocks++;
		return final_size;
	}

	return final_size;
}


void nRF24L01::Interrupt()
{
	while ( mRadio->available() ) {
		uint8_t* buf = new uint8_t[32768];
		int ret = Receive( buf, 32768 );
		if ( ret > 0 ) {
			mRxQueueMutex.lock();
			mRxQueue.emplace_back( std::make_pair( buf, ret ) );
			mRxQueueMutex.unlock();
		} else {
			delete buf;
		}
	}
}
