#include <Main.h>
#include <RendererHUDNeo.h>
#include "Camera.h"
#include "HUD.h"
#include <Stabilizer.h>
#include <IMU.h>
#include <Link.h>

HUD::HUD( const std::string& type )
	: Thread( "HUD" )
	, mGLContext( nullptr )
	, mRendererHUD( nullptr )
	, mNightMode( false )
	, mWaitTicks( 0 )
	, mShowFrequency( false )
{
	mShowFrequency = Main::instance()->config()->boolean( "hud.show_frequency", false );
	mHUDFramerate = Main::instance()->config()->integer( "hud.framerate", 60 );
	Start();
}


HUD::~HUD()
{
}


bool HUD::run()
{
	Controller* controller = Main::instance()->controller();
	Stabilizer* stablizer = Main::instance()->stabilizer();
	IMU* imu = Main::instance()->imu();
	PowerThread* powerThread = Main::instance()->powerThread();
	Camera* camera = Main::instance()->camera();

	if ( mGLContext == nullptr ) {
		mGLContext = new GLContext();
		mGLContext->Initialize( 1280, 720 );
		mWidth = mGLContext->displayWidth();
		mHeight = mGLContext->displayHeight();
		mFrameRate = std::min( camera ? camera->framerate() : 0, mGLContext->displayFrameRate() );
		if ( camera ) {
// 			mWidth = std::min( mWidth, camera->width() );
// 			mHeight = std::min( mHeight, camera->height() );
		}
	}
	if ( mRendererHUD == nullptr ) {
		Config* config = Main::instance()->config();
		Vector4i render_region = Vector4i( config->integer( "hud.top", 10 ), config->integer( "hud.bottom", 10 ), config->integer( "hud.left", 20 ), config->integer( "hud.right", 20 ) );
		mRendererHUD = new RendererHUDNeo( mGLContext->glWidth(), mGLContext->glHeight(), config->number( "hud.ratio", 1.0f ), config->integer( "hud.font_size", 60 ), render_region, config->boolean( "hud.correction", false ) );
		mRendererHUD->setStereo( config->boolean( "hud.stereo", false ) );
		mRendererHUD->set3DStrength( config->number( "hud.stereo_strength", 0.004f ) );
		mRendererHUD->setStereo( false );
		mRendererHUD->set3DStrength( 0.0f );
	}

	if ( camera and camera->nightMode() != mNightMode ) {
		mNightMode = camera->nightMode();
		mRendererHUD->setNightMode( mNightMode );
		mFrameRate = std::min( camera ? camera->framerate() : 0, mGLContext->displayFrameRate() );
	}

	glClear( GL_COLOR_BUFFER_BIT );

	DroneStats dronestats;
	LinkStats linkStats;
	memset( &dronestats, 0, sizeof(dronestats) - sizeof(std::string) - sizeof(std::vector<std::string>) );
	memset( &linkStats, 0, sizeof(linkStats) );

	dronestats.username = Main::instance()->username();
	dronestats.messages = Board::messages();
	dronestats.blackBoxId = Main::instance()->blackbox()->id();
	if ( controller ) {
		dronestats.armed = controller->armed();
		dronestats.mode = (DroneMode)stablizer->mode();
		dronestats.ping = controller->ping();
		dronestats.thrust = controller->thrust();
	}
	if ( imu ) {
		dronestats.acceleration = imu->acceleration().length();
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
	mWaitTicks = Board::WaitTick( 1000 * 1000 / mHUDFramerate, mWaitTicks );
	return true;
}
