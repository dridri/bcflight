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
// #include <iwlib.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include <gammaengine/Image.h>
#include "Stream.h"
#include "RendererHUDClassic.h"
#include "RendererHUDNeo.h"
#include "RendererHUD.h"
#include "Controller.h"

Controller _controller;
Controller* mController = &_controller;


Stream::Stream( Controller* controller, Font* font, const std::string& addr, uint16_t port )
	: Thread()
	, mInstance( nullptr )
	, mWindow( nullptr )
	, mController( controller )
	, mFont( nullptr )
{
	mFPS = 0;
	mFrameCounter = 0;

// 	mIwSocket = iw_sockets_open();
	memset( &mIwStats, 0, sizeof( mIwStats ) );

	mFPSTimer = Timer();
	mScreenTimer = Timer();
// 	Start();
}


Stream::~Stream()
{
}


bool Stream::run()
{
	if ( !mInstance and !mWindow ) {
		mInstance = Instance::Create( "flight::control", 1, true, "opengles20" );
// 		mWindow = mInstance->CreateWindow( "", -1, -1, GE::Window::Fullscreen );
#ifdef GE_ANDROID
		mWindow = mInstance->CreateWindow( "", 1280, 720, GE::Window::Fullscreen );
#else
		mWindow = mInstance->CreateWindow( "", 1280, 720, GE::Window::Nil );
#endif
		mFont = new Font( "data/RobotoCondensed-Bold.ttf", 28, 0xFF000000 );

		mRenderer = mInstance->CreateRenderer2D( mWindow->width(), mWindow->height() );
		mRenderer->setBlendingEnabled( false );

// 		mRendererHUD = new RendererHUDClassic( mInstance, mFont );
		mRendererHUD = new RendererHUDNeo( mInstance, mFont );

		mFPSTimer.Start();
		mScreenTimer.Start();
	}

	mWindow->Clear( 0x00000000 );

	mIwStats.channel = 9;
	mIwStats.level = -22;
	mIwStats.qual = 100;
	VideoStats video_stats = {
		.fps = mFPS,
	};
	mRendererHUD->PreRender( &video_stats );
	mRendererHUD->Render( mWindow, mController, &video_stats, &mIwStats );

	mWindow->SwapBuffers();
// 	printf( "%.2f\n", mWindow->fps() );
	return true;
}
