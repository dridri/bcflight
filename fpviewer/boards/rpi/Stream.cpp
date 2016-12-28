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
	: Thread( "HUD" )
	, mStereo( stereo )
	, mWidth( width_ )
	, mHeight( height_ )
	, mLink( link )
{
	srand( time( nullptr ) );

	mFPS = 0;
	mFrameCounter = 0;

	mDecoder = new VID_INTF::VideoDecode( 0, VID_INTF::VideoDecode::CodingAVC, true );

	float reduce = 0.25f;
	int y_offset = mStereo * 56 + ( reduce * mHeight * 0.1 );
	int width = mWidth / ( 1 + mStereo ) - ( reduce * mWidth * 0.1 );
	int height = mHeight - ( mStereo * 112 ) - ( reduce * mHeight * 0.2 );
	mDecoderRender1 = new VID_INTF::VideoRender( reduce * mWidth * 0.05, y_offset, width, height, true );
	mDecoderRender1->setMirror( false, true );
	if ( mStereo ) {
		mDecoderRender1->setStereo( true );
	}
	mDecoder->SetupTunnel( mDecoderRender1 );
	mDecoder->SetState( VID_INTF::Component::StateExecuting );
	mDecoderRender1->SetState( VID_INTF::Component::StateExecuting );
}


Stream::~Stream()
{
}

void Stream::setStereo( bool en )
{
	// TODO
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
	if ( frameSize > 0 ) {
		mDecoder->fillInput( buffer, frameSize );
		if ( frameSize > 41 ) {
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
