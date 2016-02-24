#include <linux/input.h>
#include <gammaengine/Debug.h>
#include "Globals.h"
#include "libbcui/Page.h"
#include "PageMain.h"
#include "PageCalibrate.h"
#include "PageSettings.h"
#include "PageNetwork.h"

using namespace BC;

::Globals* ::Globals::sInstance = nullptr;

::Globals::Globals( Instance* instance, Font* fnt )
	: BC::Globals()
	, mCursor( Vector2i( 0, 0 ) )
	, mClicking( false )
	, mCursorCounter( 0 )
	, mFont( fnt )
	, mController( nullptr )
{
	if ( !sInstance ) {
		sInstance = this;
	}

	// Wait until the system finishes starting
	usleep( 1000 * 1000 * 15 );

	mInputFD = open( "/dev/input/mouse0", O_RDONLY | O_NONBLOCK );

	mInstance = instance;
	loadConfig();
	LoadSettings( "/root/ge/settings.txt" );

	mWindow = (ProxyWindow< FramebufferWindow >*)mInstance->CreateWindow( "", 480, 320, Window::Fullscreen );
	BC::Globals::mWindow = (Window*)mWindow;
	mWindow->OpenSystemFramebuffer( "/dev/fb1", true );

	mMainRenderer = mInstance->CreateRenderer2D();
	mIcons[ "selector" ] = new Image( "data/icon_selector.png" );
	mIcons[ "PageMain" ] = new Image( "data/icon_home.png" );
	mIcons[ "PageCalibrate" ] = new Image( "data/icon_calibrate.png" );
	mIcons[ "PageNetwork" ] = new Image( "data/icon_network.png" );
	mIcons[ "PageSettings" ] = new Image( "data/icon_settings.png" );
	mIcons[ "back" ] = new Image( "data/icon_back.png" );
	mIcons[ "left" ] = new Image( "data/icon_left.png" );
	mIcons[ "right" ] = new Image( "data/icon_right.png" );
	mIcons[ "selector" ]->Resize( mWindow->width() * 0.085, mWindow->width() * 0.085 );
	mIcons[ "PageMain" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "PageCalibrate" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "PageNetwork" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "PageSettings" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "back" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "left" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "right" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );

	mScreenSize = std::min( mWindow->width(), mWindow->height() );
	mWindowSize = Vector2i( mWindow->width(), mWindow->height() );

	mPages.clear();
	setup();

	mPages[ "PageMain" ] = new PageMain();
	mPages[ "PageCalibrate" ] = new PageCalibrate();
	mPages[ "PageNetwork" ] = new PageNetwork();
	mPages[ "PageSettings" ] = new PageSettings();
}


::Globals::~Globals()
{
}


void ::Globals::RenderDrawer()
{
	auto renderer = mainRenderer();
	Image* selector = icon( "selector" );

	int icon_width = selector->width() * 1.1;
	int icon_height = selector->height() * 1.5;
	renderer->DrawLine( 16 + icon_width, 0, 0xFFFFFFFF, 16 + icon_width, window()->height(), 0xFFFFFFFF );

	int selector_y = 0;

	renderer->Draw( 8, 8 + icon_height * 0, icon( "PageMain" ) );
	if ( mPages[ "PageMain" ] == mCurrentPage ) {
		selector_y = icon_height * 0;
	}

	renderer->Draw( 8, 8 + icon_height * 1, icon( "PageCalibrate" ) );
	if ( mPages[ "PageCalibrate" ] == mCurrentPage ) {
		selector_y = icon_height * 1;
	}

	renderer->Draw( 8, 8 + icon_height * 2, icon( "PageNetwork" ) );
	if ( mPages[ "PageNetwork" ] == mCurrentPage ) {
		selector_y = icon_height * 2;
	}

	renderer->Draw( 8, 8 + icon_height * 3, icon( "PageSettings" ) );
	if ( mPages[ "PageSettings" ] == mCurrentPage ) {
		selector_y = icon_height * 3;
	}

	renderer->Draw( 8 + icon( "PageMain" )->width() / 2 - selector->width() / 2, 8 + selector_y + icon( "PageMain" )->height() / 2 - selector->height() / 2, selector );
}


bool ::Globals::PageSwitcher( int x, int y )
{
	bool ret = false;
	int icon_height = icon( "selector" )->height() * 1.5;

	if ( x >= 8 and x <= 16 + icon( "selector" )->width() ) {
		if ( y >= 8 + icon_height * 0 and y <= 8 + icon_height * 1 ) {
			ret = true;
			setCurrentPage( "PageMain" );
		} else if ( y >= 8 + icon_height * 1 and y <= 8 + icon_height * 2 ) {
			ret = true;
			setCurrentPage( "PageCalibrate" );
		} else if ( y >= 8 + icon_height * 2 and y <= 8 + icon_height * 3 ) {
			ret = true;
			setCurrentPage( "PageNetwork" );
		} else if ( y >= 8 + icon_height * 3 and y <= 8 + icon_height * 4 ) {
			ret = true;
			setCurrentPage( "PageSettings" );
		}
	}

	return ret;
}


void ::Globals::Run()
{
	uint64_t ticks = 0;
	float t = Time::GetSeconds();
	float dt = 0.0f;

	while ( !exiting() ) {
		if ( update_( t, dt ) ) {
			window()->SwapBuffers();
		}
		dt = Time::GetSeconds() - t;
		t = Time::GetSeconds();
		ticks = Time::WaitTick( 1000 / 10, ticks );
	}
}


bool ::Globals::update_( float t, float dt )
{
	struct input_event ie;
	uint8_t* ie_ptr = (uint8_t*)&ie;
	bool redraw = false;
	Page* pPage = currentPage();

	// Process all inputs event, keep only the last one
	if ( read( mInputFD, &ie, sizeof( ie ) ) >= 3 ) {
		uint8_t* ie_ptr = (uint8_t*)&ie;
		mCursor += Vector2i( (int32_t)((int8_t)ie_ptr[1]), (int32_t)((int8_t)ie_ptr[2]) );
		mCursorCounter++;
	} else {
		memset( ie_ptr, 0, sizeof( ie ) );
	}

	int32_t cursor_x = mCursor.x / 2 + 240;
	int32_t cursor_y = mCursor.y / 2 + 160;

	if ( pPage ) {
		if ( pPage->update( t, dt ) ) {
			pPage->render();
			redraw = true;
		}
		if ( mCursorCounter > 3 and ( ie_ptr[0] & 0x1 ) ) {
			pPage->touch( (float)cursor_x / (float)mWindow->width(), (float)cursor_y / (float)mWindow->height(), 1.0 );
			if ( not mClicking ) {
				pPage->click( (float)cursor_x / (float)mWindow->width(), (float)cursor_y / (float)mWindow->height(), 1.0 );
			}
		} else if ( mClicking and not ( ie_ptr[0] & 0x1 ) ) {
			pPage->touch( (float)cursor_x / (float)mWindow->width(), (float)cursor_y / (float)mWindow->height(), 0.0 );
		}
	}

	mClicking = ( ie_ptr[0] & 0x1 );
	return redraw;
}
