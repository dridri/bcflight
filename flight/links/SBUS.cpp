#include "SBUS.h"
#include <Config.h>
#include <Debug.h>

using namespace STD;


int SBUS::flight_register( Main* main )
{
	RegisterLink( "SBUS", &SBUS::Instanciate );
	return 0;
}


Link* SBUS::Instanciate( Config* config, const string& lua_object )
{
	string stype = config->String( lua_object + ".device" );
	int speed = config->Integer( lua_object + ".speed", 100000 );

	return new SBUS( stype, speed );
}


SBUS::SBUS( const string& device, int speed )
	: Link()
	, mSerial( new Serial( device, speed ) )
{
}


SBUS::~SBUS()
{
}


int SBUS::retriesCount() const
{
}


int32_t SBUS::RxLevel()
{
}


int32_t SBUS::RxQuality()
{
}


int SBUS::Connect()
{
	mConnected = true;
	return 0;
}


int32_t SBUS::Channel()
{
}


int SBUS::setBlocking( bool blocking )
{
}


void SBUS::setRetriesCount( int retries )
{
}

// 00000000 10000000 01000000 00100000 00010000 00001000 00000100 00000010 00000001 00000000

SyncReturn SBUS::Read( void* buf, uint32_t len, int32_t timeout )
{
	uint8_t data[32];
	int32_t ret = 0;
	while ( ret != 9 ) {
		int r = mSerial->Read( data, 9 - ret );
		ret += r;
		gDebug() << "r " << r << "\n";
		gDebug() << "ret " << ret << "\n";
	}

	if ( ret > 0 ) {
// 		for ( uint32_t i = 0; i < ret; i++ ) {
// 			data[i] = ~data[i];
// 		}
		uint8_t data2[32] = { 0 };
		data2[0] = data[0];
		data2[1] = ( ( data[1] << 1 & 0b11111110 ) ) | ( data[2] & 0b10000000 );
		data2[2] = ( ( data[2] << 2 & 0b11111100 ) ) | ( data[3] & 0b11000000 );
		data2[3] = ( ( data[3] << 3 & 0b11111000 ) ) | ( data[4] & 0b11100000 );
		data2[4] = ( ( data[4] << 4 & 0b11110000 ) ) | ( data[5] & 0b11110000 );
		data2[5] = ( ( data[5] << 5 & 0b11100000 ) ) | ( data[6] & 0b11111000 );
		data2[6] = ( ( data[6] << 6 & 0b11000000 ) ) | ( data[7] & 0b11111100 );
		data2[7] = ( ( data[7] << 7 & 0b10000000 ) ) | ( data[8] & 0b11111110 );
		/*
		for ( uint32_t i = 0; i < ret * 9; i++ ) {
			uint32_t pos = i / 9;
			uint32_t bit = i % 9;
			uint32_t pos2 = i / 8;
			uint32_t bit2 = i % 8;
			data2[pos2] = 0;
			data2[pos2] |= ( data[pos] >> bit ) << bit2;
		}
		*/
		gDebug() << "Received " << ret << "\n";
// 		mBuffer.insert( mBuffer.end(), data, data + ret );
		mBuffer.insert( mBuffer.end(), data2, data2 + 8 );
		gDebug() << "data first : " << hex << (int)data2[0] << "\n";
		gDebug() << "first : " << hex << (int)mBuffer[0] << "\n";
		gDebug() << "size : " << mBuffer.size() << "\n";
		if ( mBuffer[0] != 0x0F ) {
			for ( uint32_t i = 0; i < mBuffer.size(); i++ ) {
// 				gDebug() << "Buffer[" << i << "] : " << hex << (int)mBuffer[i] << dec << "\n";
				if ( i > 0 and mBuffer[i - 1] == 0x00 and mBuffer[i] == 0x0F ) {
					mBuffer = vector<uint8_t>( mBuffer.begin() + i, mBuffer.end() );
					break;
				}
			}
		} else if ( mBuffer.size() >= 25 ) {
			gDebug() << "wow : " << mBuffer.size() << "\n";

			mChannels[0]  = ((mBuffer[1]    |mBuffer[2]<<8)                 & 0x07FF);
			mChannels[1]  = ((mBuffer[2]>>3 |mBuffer[3]<<5)                 & 0x07FF);
			mChannels[2]  = ((mBuffer[3]>>6 |mBuffer[4]<<2 |mBuffer[5]<<10)  & 0x07FF);
			mChannels[3]  = ((mBuffer[5]>>1 |mBuffer[6]<<7)                 & 0x07FF);
			mChannels[4]  = ((mBuffer[6]>>4 |mBuffer[7]<<4)                 & 0x07FF);
			mChannels[5]  = ((mBuffer[7]>>7 |mBuffer[8]<<1 |mBuffer[9]<<9)   & 0x07FF);
			mChannels[6]  = ((mBuffer[9]>>2 |mBuffer[10]<<6)                & 0x07FF);
			mChannels[7]  = ((mBuffer[10]>>5|mBuffer[11]<<3)                & 0x07FF);
			mChannels[8]  = ((mBuffer[12]   |mBuffer[13]<<8)                & 0x07FF);
			mChannels[9]  = ((mBuffer[13]>>3|mBuffer[14]<<5)                & 0x07FF);
			mChannels[10] = ((mBuffer[14]>>6|mBuffer[15]<<2|mBuffer[16]<<10) & 0x07FF);
			mChannels[11] = ((mBuffer[16]>>1|mBuffer[17]<<7)                & 0x07FF);
			mChannels[12] = ((mBuffer[17]>>4|mBuffer[18]<<4)                & 0x07FF);
			mChannels[13] = ((mBuffer[18]>>7|mBuffer[19]<<1|mBuffer[20]<<9)  & 0x07FF);
			mChannels[14] = ((mBuffer[20]>>2|mBuffer[21]<<6)                & 0x07FF);
			mChannels[15] = ((mBuffer[21]>>5|mBuffer[22]<<3)                & 0x07FF);
			mChannels[16] = ((mBuffer[23])      & 0x0001) ? 2047 : 0;
			mChannels[17] = ((mBuffer[23] >> 1) & 0x0001) ? 2047 : 0;

			gDebug() << "mChannels[0] : " << mChannels[0] << "\n";
		
			mFailsafe = ((mBuffer[23] >> 3) & 0x0001) ? 1 : 0;
// 			if ((mBuffer[23] >> 2) & 0x0001) lost++; // TODO


			mBuffer = vector<uint8_t>( mBuffer.begin() + 25, mBuffer.end() );
		}
	}

	return 0;
}


SyncReturn SBUS::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
	// Haha this is SBUS, just fake it
	return len;
}
