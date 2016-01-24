#include <linux/input.h>
#include <gammaengine/Debug.h>
#include "Globals.h"
#include "libbcui/Page.h"
#include "PageMain.h"
#include "PageCalibrate.h"

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

	mInputFD = open( "/dev/input/mouse0", O_RDONLY | O_NONBLOCK );

	mInstance = instance;
	loadConfig();
	LoadSettings( "/root/ge/settings.txt" );

	mWindow = (ProxyWindow< FramebufferWindow >*)mInstance->CreateWindow( "", 480, 320, Window::Fullscreen );
	mWindow->OpenSystemFramebuffer( "/dev/fb1", true );

	mMainRenderer = mInstance->CreateRenderer2D();
	mIcons[ "selector" ] = new Image( "data/icon_selector.png" );
	mIcons[ "home" ] = new Image( "data/icon_home.png" );
	mIcons[ "calibrate" ] = new Image( "data/icon_calibrate.png" );
	mIcons[ "settings" ] = new Image( "data/icon_settings.png" );
	mIcons[ "back" ] = new Image( "data/icon_back.png" );
	mIcons[ "left" ] = new Image( "data/icon_left.png" );
	mIcons[ "right" ] = new Image( "data/icon_right.png" );
	mIcons[ "selector" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "home" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "calibrate" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "settings" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "back" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "left" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );
	mIcons[ "right" ]->Resize( mWindow->width() * 0.075, mWindow->width() * 0.075 );

	mScreenSize = std::min( mWindow->width(), mWindow->height() );
	mWindowSize = Vector2i( mWindow->width(), mWindow->height() );

	mPages.clear();
	setup();

	mPages[ "PageMain" ] = new PageMain();
	mPages[ "PageCalibrate" ] = new PageCalibrate();
}


::Globals::~Globals()
{
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
		if ( mCursorCounter > 3 and ( ie_ptr[0] & 0x1 ) and not mClicking ) {
			pPage->click( (float)cursor_x / (float)mWindow->width(), (float)cursor_y / (float)mWindow->height(), 1.0 );
		}
	}

	mClicking = ( ie_ptr[0] & 0x1 );
	return redraw;
}
