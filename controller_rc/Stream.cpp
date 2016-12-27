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
#include <wiringPi.h>
#include <iostream>
#include <links/RawWifi.h>
#include <links/Socket.h>
#include "Stream.h"
#include "Controller.h"
#include "RendererHUDNeo.h"

#define UDPLITE_SEND_CSCOV   10 /* sender partial coverage (as sent)      */
#define UDPLITE_RECV_CSCOV   11 /* receiver partial coverage (threshold ) */

Stream::Stream( Controller* controller, Link* link, uint32_t width, uint32_t height, bool stereo )
	: Thread( "HUD" )
	, mController( controller )
	, mRendererHUD( nullptr )
	, mRenderHUD( true )
	, mStereo( stereo )
	, mWidth( width )
	, mHeight( height )
	, mLink( link )
	, mDecodeThread( nullptr )
	, mDecodeInput( nullptr )
	, mEGLVideoImage( 0 )
	, mVideoImage( nullptr )
	, mEGLContext( 0 )
{
	srand( time( nullptr ) );

	mFPS = 0;
	mFrameCounter = 0;
	mDecodeLen = 0;

	mIwSocket = iw_sockets_open();
	memset( &mIwStats, 0, sizeof( mIwStats ) );

	mSignalThread = new HookThread< Stream >( "signal-quality", this, &Stream::SignalThreadRun );
	mSignalThread->Start();
}


Stream::~Stream()
{
// 	vc_dispmanx_resource_delete( mScreenshot.resource );
	vc_dispmanx_display_close( mDisplay );
}

void Stream::setStereo( bool en )
{
/*
	usleep( 1000 * 500 );
	if ( not this->running() ) {
		return;
	}
	while ( not this->running() or not mRendererHUD ) {
		usleep( 1000 );
	}
	mStereo = en;
	mRendererHUD->setStereo( en );
*/
}


bool Stream::run()
{
	if ( mEGLContext == 0 ) {
		EGLint attribList[] =
		{
			EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
			EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
			EGL_RED_SIZE,          8,
			EGL_GREEN_SIZE,        8,
			EGL_BLUE_SIZE,         8,
			EGL_ALPHA_SIZE,        8,
			EGL_DEPTH_SIZE,        16,
			EGL_STENCIL_SIZE,      0,
			EGL_NONE
		};

		EGLint numConfigs;
		EGLint majorVersion;
		EGLint minorVersion;
		EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
		mEGLDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
		eglInitialize( mEGLDisplay, &majorVersion, &minorVersion );
		eglGetConfigs( mEGLDisplay, nullptr, 0, &numConfigs );
		eglChooseConfig( mEGLDisplay, attribList, &mEGLConfig, 1, &numConfigs );
		mEGLContext = eglCreateContext( mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, contextAttribs );

		mLayerDisplay = CreateNativeWindow( 2 );

		const EGLint egl_surface_attribs[] = {
			EGL_RENDER_BUFFER, EGL_BACK_BUFFER/* + 1 => disable double-buffer*/,
			EGL_NONE,
		};
		mEGLSurface = eglCreateWindowSurface( mEGLDisplay, mEGLConfig, reinterpret_cast< EGLNativeWindowType >( &mLayerDisplay ), egl_surface_attribs );
		eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_WIDTH, (EGLint*)&mEGLWidth );
		eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_HEIGHT, (EGLint*)&mEGLHeight );
		eglMakeCurrent( mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext );

		std::cout << "OpenGL version : " << glGetString( GL_VERSION ) << "\n";
		glViewport( 0, 0, mEGLWidth, mEGLHeight );
		glEnable( GL_CULL_FACE );
		glFrontFace( GL_CCW );
		glCullFace( GL_BACK );
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		mRendererHUD = new RendererHUDNeo( mEGLWidth, mEGLHeight );

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

// 		mEGLRender = new VID_INTF::EGLRender( true );

		mDecoder->SetupTunnel( mDecoderRender1 );
// 		mDecoder->SetupTunnel( mEGLRender );
		mDecoder->SetState( VID_INTF::Component::StateExecuting );
		mDecoderRender1->SetState( VID_INTF::Component::StateExecuting );

		mDecodeThread = new HookThread< Stream >( "decoder", this, &Stream::DecodeThreadRun );
// 		mDecodeThread->setPriority( 99 );
		mDecodeThread->Start();

		return false;

		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	}
/*
	if ( mVideoImage == nullptr and mDecoder->width() > 0 and mDecoder->height() > 0 ) {
		float reduce = 0.25f;
		int y_offset = mStereo * 56 + ( reduce * mEGLHeight * 0.1 );
		int width = mEGLWidth / ( 1 + mStereo ) - ( reduce * mEGLWidth * 0.1 );
		int height = mEGLHeight - ( mStereo * 112 ) - ( reduce * mEGLHeight * 0.2 );
		mVideoImage = new DecodedImage( mDecoder->width(), mDecoder->height(), 0xFF000000 );
		mVideoImage->setDrawCoordinates( reduce * mEGLWidth * 0.05, y_offset, width, height );
		mEGLVideoImage = eglCreateImageKHR( eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)mVideoImage->texture(), nullptr );
		if ( mEGLVideoImage == EGL_NO_IMAGE_KHR ) {
			printf("eglCreateImageKHR failed.\n");
			exit(1);
		}
		mEGLRender->setEGLImage( mEGLVideoImage );
		mEGLRender->SetState( VID_INTF::Component::StateExecuting );
		printf( "Video should be OK (%d x %d)\n", mDecoder->width(), mDecoder->height() );
	}
*/
	EnterCritical();
	glClear( GL_COLOR_BUFFER_BIT );
	ExitCritical();

/*
	if ( mVideoImage != nullptr and mEGLVideoImage != 0 ) {
// 		EnterCritical();
		mEGLRender->sinkToEGL();
// 		ExitCritical();
		mVideoImage->Draw( mRendererHUD );
	}
*/

	if ( mRenderHUD ) {
		VideoStats video_stats = {
			.fps = mFPS,
		};
		mRendererHUD->PreRender( &video_stats );
		mRendererHUD->Render( mController, &video_stats, &mIwStats );
	}

	uint64_t t = Thread::GetTick();
	uint32_t minuts = t / ( 1000 * 60 );
	uint32_t seconds = ( t / 1000 ) % 60;
	uint32_t ms = t % 1000;
	mRendererHUD->RenderText( 200, 400, std::to_string(minuts) + ":" + std::to_string(seconds) + ":" + std::to_string(ms), 0xFFFFFFFF );

/*
	if ( mSecondTimer.ellapsed() >= 1000 ) {
		// TEST : NIGHT MODE
		vc_dispmanx_snapshot( mDisplay, mScreenshot.resource, DISPMANX_NO_ROTATE );
		vc_dispmanx_resource_read_data( mScreenshot.resource, &mScreenshot.rect, mScreenshot.image, 1920 / 2 * 3 );
		uint32_t r = 0;
		uint32_t g = 0;
		uint32_t b = 0;
		for ( uint32_t y = 0; y < 1080; y += 36 ) {
			for ( uint32_t x = 0; x < 1920 / 2; x += 32 ) {
				r += ((uint8_t*)mScreenshot.image)[ ( y * 1920 / 2 + x ) * 3 + 0 ];
				g += ((uint8_t*)mScreenshot.image)[ ( y * 1920 / 2 + x ) * 3 + 1 ];
				b += ((uint8_t*)mScreenshot.image)[ ( y * 1920 / 2 + x ) * 3 + 2 ];
			}
		}
		mRendererHUD->setNightMode( r / 900 < 32 and g / 900 < 32 and b / 900 < 32 );
	}
*/
	EnterCritical();
	eglSwapBuffers( mEGLDisplay, mEGLSurface );
	ExitCritical();
	return true;
}


bool Stream::SignalThreadRun()
{
// 	return false; // TEST
	iwstats stats;
	wireless_config info;
	iwrange range;
	memset( &stats, 0, sizeof( stats ) );
	if ( iw_get_stats( mIwSocket, "wlan0", &stats, nullptr, 0 ) == 0 ) {
		mIwStats.qual = (int)stats.qual.qual * 100 / 70;
		union { int8_t s; uint8_t u; } conv = { .u = stats.qual.level };
		mIwStats.level = conv.s;
		mIwStats.noise = stats.qual.noise;
	}
	if ( iw_get_basic_config( mIwSocket, "wlan0", &info ) == 0 ) {
		if ( iw_get_range_info( mIwSocket, "wlan0", &range ) == 0 ) {
			mIwStats.channel = iw_freq_to_channel( info.freq, &range );
		}
	}
	if ( dynamic_cast< RawWifi* >( mLink ) != nullptr and mController != nullptr ) {
		mIwStats.source = 'R';
		mIwStats.qual = mController->link()->RxQuality();
		if ( (int)mController->droneRxQuality() < mIwStats.qual ) {
			mIwStats.source = 'T';
			mIwStats.qual = mController->droneRxQuality();
		}
		if ( mLink->RxQuality() < mIwStats.qual ) {
			mIwStats.source = 'V';
			mIwStats.qual = mLink->RxQuality();
		}
		mIwStats.level = dynamic_cast< RawWifi* >( mLink )->level();
// 		mIwStats.channel = dynamic_cast< RawWifi* >( mLink )->channel();
	}
/*
	if ( not mDecodeThread or not mDecodeThread->running() ) {
		mDecodeThread = new HookThread< Stream >( "decoder", this, &Stream::DecodeThreadRun );
		mDecodeThread->setPriority( 99 );
		mDecodeThread->Start();
	}
	*/
	usleep( 1000 * 250 );
	return true;
}


bool Stream::DecodeThreadRun()
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
// 	Packet* packet = new Packet;
// 	packet->size = sizeof(packet->data);
// 	uint8_t* buffer = packet->data;
// 	uint32_t bufsize = packet->size;

	frameSize = mLink->Read( buffer, bufsize, 0 );
	if ( frameSize > 0 ) {
		EnterCritical();
		mDecoder->fillInput( buffer, frameSize );
// 		packet->size = frameSize;
// 		mPacketsQueue.emplace_back( packet );
		ExitCritical();
		if ( frameSize > 41 ) {
			mFrameCounter++;
		}
		mBitrateCounter += frameSize;
		frameSize = 0;
	}

	if ( Thread::GetTick() - mFPSTick >= 1000 ) {
		mFPSTick = Thread::GetTick();
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


EGL_DISPMANX_WINDOW_T Stream::CreateNativeWindow( int layer )
{
	EGL_DISPMANX_WINDOW_T nativewindow;
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	uint32_t display_width;
	uint32_t display_height;

	int ret = graphics_get_display_size( 5, &display_width, &display_height );
	std::cout << "display size ( " << ret << " ) : " << display_width << " x "<< display_height << "\n";

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = display_width;
	dst_rect.height = display_height;

	// Force this layer to 720p for better performances (automatically upscaled by dispman)
	display_width = 1280;
	display_height = 720;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = display_width << 16;
	src_rect.height = display_height << 16;

	mDisplay = vc_dispmanx_display_open( 5 );
	dispman_update = vc_dispmanx_update_start( 0 );

	VC_DISPMANX_ALPHA_T alpha;
	alpha.flags = (DISPMANX_FLAGS_ALPHA_T)( DISPMANX_FLAGS_ALPHA_FROM_SOURCE );
	alpha.opacity = 0;

	dispman_element = vc_dispmanx_element_add( dispman_update, mDisplay, layer, &dst_rect, 0, &src_rect, DISPMANX_PROTECTION_NONE, &alpha, 0, DISPMANX_NO_ROTATE );
	nativewindow.element = dispman_element;
	nativewindow.width = display_width;
	nativewindow.height = display_height;
	vc_dispmanx_update_submit_sync( dispman_update );

// 	mScreenshot.image = new uint8_t[ 1920 / 2 * 1080 * 3 ];
// 	mScreenshot.resource = vc_dispmanx_resource_create( VC_IMAGE_RGB888, 1920 / 2, 1080, &mScreenshot.vc_image_ptr );
// 	vc_dispmanx_rect_set( &mScreenshot.rect, 0, 0, 1920 / 2, 1080 );

	return nativewindow;
}

