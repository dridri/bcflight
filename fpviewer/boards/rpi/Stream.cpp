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

Stream::Stream( Link* link, uint32_t width_, uint32_t height_, float zoom, bool stretch, bool stereo, bool direct_render )
	: Thread( "Stream" )
	, mStereo( stereo )
	, mDirectRender( direct_render )
	, mScreenWidth( width_ )
	, mScreenHeight( height_ )
	, mWidth( 0 )
	, mHeight( 0 )
	, mZoom( zoom )
	, mStretch( stretch )
	, mLink( link )
	, mDecoder( nullptr )
	, mDecoderRender( nullptr )
	, mEGLRender( nullptr )
	, mEGLVideoImage( nullptr )
	, mGLImage( nullptr )
{
	srand( time( nullptr ) );

	mFPS = 0;
	mFrameCounter = 0;

	mDecoder = new VID_INTF::VideoDecode( 0, VID_INTF::VideoDecode::CodingAVC, true );
	if ( direct_render ) {
// 		float reduce = 1.0f - zoom;
// 		int y_offset = mStereo * 56 + ( reduce * height_ * 0.1 );
// 		int width = width_ / ( 1 + mStereo ) - ( reduce * width_ * 0.1 );
// 		int height = height_ - ( mStereo * 112 ) - ( reduce * height_ * 0.2 );

			int base_width = mScreenWidth;
			int base_height = mScreenHeight;
			if ( not mStretch ) {
// 				base_width = mWidth;
				base_height = base_height * mWidth / base_width;
			}
			int width = base_width * mZoom / ( mStereo + 1 );
			int height = base_height * mZoom;
			int x_offset = mScreenWidth / ( 2 * ( mStereo + 1 ) ) - width / 2;
			int y_offset = mScreenHeight / 2 - height / 2;

		mDecoderRender = new VID_INTF::VideoRender( x_offset, y_offset, width, height, false );
		mDecoderRender->setMirror( false, false );
		if ( mStereo ) {
			mDecoderRender->setStereo( true );
		}
		mDecoder->SetupTunnel( mDecoderRender );
		mDecoder->SetState( VID_INTF::Component::StateExecuting );
		mDecoderRender->SetState( VID_INTF::Component::StateExecuting );
	} else {
		mEGLRender = new VID_INTF::EGLRender( true );
		mDecoder->SetupTunnel( mEGLRender );
		mDecoder->SetState( VID_INTF::Component::StateExecuting );
	}

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
		if ( frameSize <= 128 ) {
// 			printf( "Header received (size: %d)\n", frameSize );
			if ( buffer[0] == 0x00 and not mDecoder->valid() and not corrupted ) {
				bool already_received = false;
				if ( mHeadersReceived.size() > 0 ) {
					for ( auto hdr : mHeadersReceived ) {
						if ( hdr == (uint32_t)frameSize ) {
							already_received = true;
							break;
						}
					}
				}
// 				if ( not already_received ) {
					printf( "Processing header (%d)\n", frameSize );
					mDecoder->fillInput( buffer, frameSize, corrupted );
					mHeadersReceived.emplace_back( frameSize );
// 				}
			}
		} else {
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


void Stream::Render( RendererHUD* renderer )
{
	if ( mDirectRender ) {
		// Nothing to do
	} else {
		if ( mGLImage == nullptr and width() > 0 and height() > 0 ) {
			int base_width = mScreenWidth;
			int base_height = mScreenHeight;
			if ( not mStretch ) {
// 				base_width = mWidth;
				base_height = base_height * mWidth / base_width;
			}
			int width = base_width * mZoom / ( mStereo + 1 );
			int height = base_height * mZoom;
			int x_offset = mScreenWidth / ( 2 * ( mStereo + 1 ) ) - width / 2;
			int y_offset = mScreenHeight / 2 - height / 2;
			printf( "GLImage coordinates : %d %d %d %d\n", x_offset, y_offset, width, height );
			mGLImage = new GLImage( Stream::width(), Stream::height() );
			mGLImage->setDrawCoordinates( x_offset * 1280.0f / mScreenWidth, y_offset * 720.0f / mScreenHeight, width * 1280.0f / mScreenWidth, height * 720.0f / mScreenHeight );
			mEGLVideoImage = eglCreateImageKHR( eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)mGLImage->texture(), nullptr );
			if ( mEGLVideoImage == EGL_NO_IMAGE_KHR ) {
				printf("eglCreateImageKHR failed.\n");
				exit(1);
			}
			OMX_ERRORTYPE err = mEGLRender->setEGLImage( mEGLVideoImage );
			mEGLRender->SetState( VID_INTF::Component::StateExecuting );
			printf( "Video should be OK (%d x %d)\n", mDecoder->width(), mDecoder->height() );
		}
		if ( mEGLRender and mEGLRender->state() == VID_INTF::Component::StateExecuting ) {
			OMX_ERRORTYPE err = mEGLRender->sinkToEGL();
		}
		if ( mGLImage ) {
			mGLImage->Draw( renderer );
		}
	}
}
