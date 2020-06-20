#if ( BUILD_HUD == 1 )

#include <Main.h>
#include <RendererHUDNeo.h>
#include "Camera.h"
#include "HUD.h"
#include <Stabilizer.h>
#include <IMU.h>
#include <Link.h>
#include <Config.h>

HUD::HUD( const string& type )
	: Thread( "HUD" )
	, mGLContext( nullptr )
	, mRendererHUD( nullptr )
	, mNightMode( false )
	, mWaitTicks( 0 )
	, mShowFrequency( false )
	, mAccelerationAccum( 0.0f )
{
	mShowFrequency = Main::instance()->config()->Boolean( "hud.show_frequency", false );
	mHUDFramerate = Main::instance()->config()->Integer( "hud.framerate", 60 );
	setFrequency( mHUDFramerate );
	Start();
}


HUD::~HUD()
{
}


bool HUD::run()
{
	Controller* controller = Main::instance()->controller();
	Stabilizer* stabilizer = Main::instance()->stabilizer();
	IMU* imu = Main::instance()->imu();
	PowerThread* powerThread = Main::instance()->powerThread();
	Camera* camera = Main::instance()->camera();

	if ( mGLContext == nullptr ) {
		mGLContext = new GLContext();
		mGLContext->Initialize( 1280, 720 );
		mWidth = mGLContext->displayWidth();
		mHeight = mGLContext->displayHeight();
		mFrameRate = min( camera ? camera->framerate() : 0, mGLContext->displayFrameRate() );
		if ( camera ) {
// 			mWidth = min( mWidth, camera->width() );
// 			mHeight = min( mHeight, camera->height() );
		}
	}
	if ( mRendererHUD == nullptr ) {
		Config* config = Main::instance()->config();
		Vector4i render_region = Vector4i( config->Integer( "hud.top", 10 ), config->Integer( "hud.bottom", 10 ), config->Integer( "hud.left", 20 ), config->Integer( "hud.right", 20 ) );
		mRendererHUD = new RendererHUDNeo( mGLContext->glWidth(), mGLContext->glHeight(), config->Number( "hud.ratio", 1.0f ), config->Integer( "hud.font_size", 60 ), render_region, config->Boolean( "hud.correction", false ) );
		mRendererHUD->setStereo( config->Boolean( "hud.stereo", false ) );
		mRendererHUD->set3DStrength( config->Number( "hud.stereo_strength", 0.004f ) );
		mRendererHUD->setStereo( false );
		mRendererHUD->set3DStrength( 0.0f );
	}

	if ( camera and camera->nightMode() != mNightMode ) {
		mNightMode = camera->nightMode();
		mRendererHUD->setNightMode( mNightMode );
		mFrameRate = min( camera ? camera->framerate() : 0, mGLContext->displayFrameRate() );
	}

	glClear( GL_COLOR_BUFFER_BIT );

	DroneStats dronestats;
	LinkStats linkStats;
	memset( &dronestats, 0, sizeof(dronestats) - sizeof(string) - sizeof(vector<string>) );
	memset( &linkStats, 0, sizeof(linkStats) );

	dronestats.username = Main::instance()->username();
	dronestats.messages = Board::messages();
	dronestats.blackBoxId = Main::instance()->blackbox()->id();
	if ( controller and stabilizer ) {
		dronestats.armed = stabilizer->armed();
		dronestats.mode = (DroneMode)stabilizer->mode();
		dronestats.ping = controller->ping();
		dronestats.thrust = stabilizer->thrust();
	}
	if ( imu ) {
		mAccelerationAccum = ( mAccelerationAccum * 0.95f + imu->acceleration().length() * 0.05f );
		dronestats.acceleration = mAccelerationAccum;
		dronestats.rpy = imu->RPY();
	}
	if ( powerThread ) {
		dronestats.batteryLevel = powerThread->BatteryLevel();
		dronestats.batteryVoltage = powerThread->VBat();
		dronestats.batteryTotalCurrent = (uint32_t)( powerThread->CurrentTotal() * 1000 );
	}
	if ( controller and controller->link() ) {
		linkStats.qual = controller->link()->RxQuality();
		linkStats.level = controller->link()->RxLevel();
		linkStats.noise = 0;
		linkStats.channel = ( mShowFrequency ? controller->link()->Frequency() : controller->link()->Channel() );
		linkStats.source = 0;
	}

	VideoStats video_stats = {
		.width = (int)mWidth,
		.height = (int)mHeight,
		.fps = (int)mFrameRate,
	};
	if ( camera ) {
		strncpy( video_stats.whitebalance, camera->whiteBalance().c_str(), 32 );
		strncpy( video_stats.exposure, camera->exposureMode().c_str(), 32 );
		video_stats.photo_id = camera->getLastPictureID();
	}

	mRendererHUD->PreRender();
	mRendererHUD->Render( &dronestats, 0.0f, &video_stats, &linkStats );

	if ( false ) {
		uint32_t time = Board::GetTicks() / 1000;
		uint32_t minuts = time / ( 1000 * 60 );
		uint32_t seconds = ( time / 1000 ) % 60;
		uint32_t ms = time % 1000;
		char txt[256];
		sprintf( txt, "%02d:%02d:%03d", minuts, seconds, ms );
		mRendererHUD->RenderText( 1280 * 0.5, 100, txt, 0xFFFFFFFF, 4.0f, true );
	}

 	mGLContext->SwapBuffers();
	return true;
}

#endif // ( BUILD_HUD == 1 )
