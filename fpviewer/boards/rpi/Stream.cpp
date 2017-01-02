/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
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

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <iwlib.h>
#include <iostream>
#include <links/RawWifi.h>
#include <links/Socket.h>
#include "Stream.h"

#define UDPLITE_SEND_CSCOV   10 /* sender partial coverage (as sent)      */
#define UDPLITE_RECV_CSCOV   11 /* receiver partial coverage (threshold ) */

Stream::Stream( Link* link, uint32_t width_, uint32_t height_, bool stereo )
	: Thread( "Stream" )
	, mStereo( stereo )
	, mWidth( 0 )
	, mHeight( 0 )
	, mLink( link )
{
	srand( time( nullptr ) );

	mFPS = 0;
	mFrameCounter = 0;

	mDecoder = new VID_INTF::VideoDecode( 0, VID_INTF::VideoDecode::CodingAVC, true );

	float reduce = 0.25f;
	int y_offset = mStereo * 56 + ( reduce * height_ * 0.1 );
	int width = width_ / ( 1 + mStereo ) - ( reduce * width_ * 0.1 );
	int height = height_ - ( mStereo * 112 ) - ( reduce * height_ * 0.2 );
	mDecoderRender1 = new VID_INTF::VideoRender( reduce * width_ * 0.05, y_offset, width, height, true );
	mDecoderRender1->setMirror( false, true );
	if ( mStereo ) {
		mDecoderRender1->setStereo( true );
	}
	mDecoder->SetupTunnel( mDecoderRender1 );
	mDecoder->SetState( VID_INTF::Component::StateExecuting );
	mDecoderRender1->SetState( VID_INTF::Component::StateExecuting );

	setPriority( 99 );
}


Stream::~Stream()
{
}

void Stream::setStereo( bool en )
{
	// TODO
}


uint32_t Stream::width()
{
	if ( mWidth == 0 and mDecoder->valid() ) {
		mWidth = mDecoder->width();
	}
	return mWidth;
}


uint32_t Stream::height()
{
	if ( mHeight == 0 and mDecoder->valid() ) {
		mHeight = mDecoder->height();
	}
	return mHeight;
}


bool Stream::run()
{
	int32_t frameSize = 0;

	if ( not mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			uint32_t uid = htonl( 0x12345678 );
			mLink->Write( &uid, sizeof(uid), 0 );
		} else {
			usleep( 1000 * 1000 );
		}
		return true;
	}

	const uint32_t bufsize = 32768;
	uint8_t buffer[bufsize] = { 0 };

	frameSize = mLink->Read( buffer, bufsize, 0 );

	bool corrupted = false;
	if ( dynamic_cast< RawWifi* >( mLink ) != nullptr ) {
		corrupted = dynamic_cast< RawWifi* >( mLink )->lastIsCorrupt();
	}

	if ( frameSize > 0 ) {
		// h264 headers
		if ( frameSize <= 42 and buffer[0] == 0x00 and not mDecoder->valid() ) {
			if ( not corrupted ) {
				bool already_received = false;
				if ( mHeadersReceived.size() > 0 ) {
					for ( auto hdr : mHeadersReceived ) {
						if ( hdr == frameSize ) {
							already_received = true;
							break;
						}
					}
				}
				if ( not already_received ) {
					printf( "Processing header (%d)\n", frameSize );
					mDecoder->fillInput( buffer, frameSize, corrupted );
					mHeadersReceived.emplace_back( frameSize );
				}
			}
		} else if ( frameSize > 42 ) {
			mDecoder->fillInput( buffer, frameSize, corrupted );
			mFrameCounter++;
		}
		mBitrateCounter += frameSize;
		frameSize = 0;
	}

	if ( GetTick() - mFPSTick >= 1000 ) {
		mFPSTick = GetTick();
		mFPS = mFrameCounter;
		mFrameCounter = 0;
		mBitrateCounter = 0;

		if ( dynamic_cast< Socket* >( mLink ) != nullptr ) {
			uint32_t uid = htonl( 0x12345678 );
			mLink->Write( &uid, sizeof(uid), 0 );
		}
	}

	return true;
}
