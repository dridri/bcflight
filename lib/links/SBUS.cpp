#include "SBUS.h"
#include <Config.h>
#include <Debug.h>
#include <Board.h>
#include <ControllerBase.h>

using namespace std;


SBUS::SBUS( Bus* bus, uint8_t armChannel, uint8_t thrustChannel, uint8_t rollChannel, uint8_t pitchChannel, uint8_t yawChannel )
	: Link()
	, mBus( bus )
	, mArmChannel( armChannel )
	, mThrustChannel( thrustChannel )
	, mRollChannel( rollChannel )
	, mPitchChannel( pitchChannel )
	, mYawChannel( yawChannel )
	, mCalibrating( false )
{
}


SBUS::SBUS()
	: mBus( nullptr )
	, mArmChannel( 0 )
	, mThrustChannel( 0 )
	, mRollChannel( 0 )
	, mPitchChannel( 0 )
	, mYawChannel( 0 )
	, mCalibrating( false )
{
}


SBUS::~SBUS()
{
}


int SBUS::retriesCount() const
{
	return 0;
}


int32_t SBUS::RxLevel()
{
	return 0;
}


int32_t SBUS::RxQuality()
{
	return 0;
}


int SBUS::Connect()
{
	mConnected = true;
	return 0;
}


int32_t SBUS::Channel()
{
	return 0;
}


int SBUS::setBlocking( bool blocking )
{
	(void)blocking;
	return 0;
}


void SBUS::setRetriesCount( int retries )
{
}

// 00000000 10000000 01000000 00100000 00010000 00001000 00000100 00000010 00000001 00000000

uint32_t X = 0;

SyncReturn SBUS::Read( void* buf, uint32_t len, int32_t timeout )
{
	vector< uint8_t > mBuffer;
	uint8_t data[32];
	int32_t ret = 0;
	int32_t i = 0;

	while ( mBuffer.size() < 25 ) {
		ret = mBus->Read( data, 25 - mBuffer.size() );
		i = 0;
		if ( mBuffer.size() == 0 ) {
			while ( i < ret and data[i] != 0x0F ) {
				i++;
			}
		}
		if ( i < ret ) {
			mBuffer.insert( mBuffer.end(), &data[i], &data[ret] );
		}
	}
/*
	for ( uint32_t i = 0; i < 25; i += 8 ) {
		for ( uint32_t j = 0; j < 8 && i + j < 25; j++ ) {
			printf( "%02X ", mBuffer[i + j] );
		}
		printf( "\n" );
	}
*/
	if ( mBuffer[24] != 0x00 ) {
//		printf( "error (%02X)\n", mBuffer[24] );
	} else {
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
/*
		for ( uint32_t c = 0; c < 18; c++ ) {
			gDebug() << "mChannels[" << c << "] : " << mChannels[c];
		}
*/
		mFailsafe = ((mBuffer[23] >> 3) & 0x0001) ? true : false;
// 		if ((mBuffer[23] >> 2) & 0x0001) lost++; // TODO

		ControllerBase::Controls controls;
		float f_thrust = ( ( (float)mChannels[mThrustChannel] - 172.0f ) / 1640.0f );
		float f_yaw = ( ( (float)mChannels[mYawChannel] - 992.0f ) / 820.0f );
		float f_pitch = ( ( (float)mChannels[mPitchChannel] - 992.0f ) / 820.0f );
		float f_roll = ( ( (float)mChannels[mRollChannel] - 992.0f ) / 820.0f );
		if ( f_thrust >= 0.0f and f_thrust <= 1.0f ) {
			controls.thrust = (int8_t)( max( 0, min( 127, (int32_t)( f_thrust * 127.0f ) ) ) );
		}
		if ( f_yaw >= -1.0f and f_yaw <= 1.0f ) {
			if ( fabsf( f_yaw ) <= 0.025f ) {
				f_yaw = 0.0f;
			} else if ( f_yaw < 0.0f ) {
				f_yaw += 0.025f;
			} else if ( f_yaw > 0.0f ) {
				f_yaw -= 0.025f;
			}
			f_yaw *= 1.0f / ( 1.0f - 0.025f );
			controls.yaw = (int8_t)( max( -127, min( 127, (int32_t)( f_yaw * 127.0f ) ) ) );
		}
		if ( f_pitch >= -1.0f and f_pitch <= 1.0f ) {
			if ( fabsf( f_pitch ) <= 0.025f ) {
				f_pitch = 0.0f;
			} else if ( f_pitch < 0.0f ) {
				f_pitch += 0.025f;
			} else if ( f_pitch > 0.0f ) {
				f_pitch -= 0.025f;
			}
			f_pitch *= 1.0f / ( 1.0f - 0.025f );
			controls.pitch = (int8_t)( max( -127, min( 127, (int32_t)( f_pitch * 127.0f ) ) ) );
		}
		if ( f_roll >= -1.0f and f_roll <= 1.0f ) {
			if ( fabsf( f_roll ) <= 0.025f ) {
				f_roll = 0.0f;
			} else if ( f_roll < 0.0f ) {
				f_roll += 0.025f;
			} else if ( f_roll > 0.0f ) {
				f_roll -= 0.025f;
			}
			f_roll *= 1.0f / ( 1.0f - 0.025f );
			controls.roll = (int8_t)( max( -127, min( 127, (int32_t)( f_roll * 127.0f ) ) ) );
		}
		controls.arm = ( mChannels[mArmChannel] > 500 ); // TBD : disarm if mFailsafe is true ?

		Packet txFrame;
		if ( not controls.arm and ( f_thrust >= 0.85f and f_pitch >= 0.85f ) ) {
			if ( not mCalibrating ) {
				mCalibrating = true;
				struct {
					uint32_t cmd = 0;
					uint32_t full_recalibration = 1;
					float current_altitude = 0.0f;
				} calibrate_cmd;
				calibrate_cmd.cmd = board_htonl( ControllerBase::CALIBRATE );
				txFrame.Write( (uint8_t*)&calibrate_cmd, sizeof( calibrate_cmd ) );
			}
		} else {
			mCalibrating = false;
		}
		txFrame.WriteU8( ControllerBase::CONTROLS );
		txFrame.Write( (uint8_t*)&controls, sizeof(controls) );

		memcpy( buf, txFrame.data().data(), txFrame.data().size() );
		return txFrame.data().size();
	}
	return 0;
}


SyncReturn SBUS::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
	(void)buf;
	(void)ack;
	(void)timeout;
	// Haha this is SBUS, just fake it
	return len;
}
