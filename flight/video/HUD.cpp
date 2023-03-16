#if ( BUILD_HUD == 1 )

#include <Main.h>
#include <RendererHUDNeo.h>
#include "Camera.h"
#include "HUD.h"
#include <GLContext.h>
#include <Stabilizer.h>
#include <IMU.h>
#include <Link.h>
#include <Config.h>


HUD::HUD()
	: Thread( "HUD" )
	, mGLContext( nullptr )
	, mRendererHUD( nullptr )
	, mNightMode( false )
	, mWaitTicks( 0 )
	, mShowFrequency( false )
	, mAccelerationAccum( 0.0f )
	, mFrameTop( 10 )
	, mFrameBottom( 10 )
	, mFrameLeft( 20 )
	, mFrameRight( 20 )
	, mRatio( 1.0f )
	, mFontSize( 60 )
	, mCorrection( false )
	, mStereo( false )
	, mStereoStrength( 0.004f )
{
	mShowFrequency = false;
	mHUDFramerate = 30;
	setFrequency( mHUDFramerate );

	mGLContext = GLContext::instance();

	mGLContext->runOnGLThread( [this]() {
		Config* config = Main::instance()->config();
		Vector4i render_region = Vector4i( mFrameTop, mFrameBottom, mFrameLeft, mFrameRight );
		mRendererHUD = new RendererHUDNeo( mGLContext->glWidth(), mGLContext->glHeight(), mRatio, mFontSize, render_region, mCorrection );
		mRendererHUD->setStereo( mStereo );
		mRendererHUD->set3DStrength( mStereoStrength );
		mRendererHUD->setStereo( false );
		mRendererHUD->set3DStrength( 0.0f );
	}, true );

	mGLContext->addLayer( [this]() {
		mRendererHUD->PreRender();
		mRendererHUD->Render( &mDroneStats, 0.0f, &mVideoStats, &mLinkStats );
		mImagesMutex.lock();
		for ( std::pair< uintptr_t, Image > image : mImages ) {
			Image img = image.second;
			mRendererHUD->RenderImage( img.x, img.y, img.width, img.height, img.img );
		}
		mImagesMutex.unlock();

		if ( false ) {
			uint32_t time = Board::GetTicks() / 1000;
			uint32_t minuts = time / ( 1000 * 60 );
			uint32_t seconds = ( time / 1000 ) % 60;
			uint32_t ms = time % 1000;
			char txt[256];
			sprintf( txt, "%02d:%02d:%03d", minuts, seconds, ms );
			mRendererHUD->RenderText( 200, 200, txt, 0xFFFFFFFF, 4.0f, true );
		}
	});

	Start();
}


HUD::~HUD()
{
}


bool HUD::run()
{
	if ( frequency() != mHUDFramerate ) {
		setFrequency( mHUDFramerate );
	}

	Controller* controller = Main::instance()->controller();
	Stabilizer* stabilizer = Main::instance()->stabilizer();
	IMU* imu = Main::instance()->imu();
	PowerThread* powerThread = Main::instance()->powerThread();
	Camera* camera = Main::instance()->camera();

	mFrameRate = min( camera ? camera->framerate() : 0, mGLContext->displayFrameRate() );

	if ( mNightMode != mRendererHUD->nightMode() ) {
		mRendererHUD->setNightMode( mNightMode );
	}

	glClear( GL_COLOR_BUFFER_BIT );

	DroneStats dronestats;
	LinkStats linkStats;

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

	mDroneStats = dronestats;
	mLinkStats = linkStats;

	if ( camera ) {
		mVideoStats = VideoStats( (int)camera->width(), (int)camera->height(), (int)camera->framerate() );
		// strncpy( video_stats.whitebalance, camera->whiteBalance().c_str(), 32 );
		// strncpy( video_stats.exposure, camera->exposureMode().c_str(), 32 );
		// video_stats.photo_id = camera->getLastPictureID();
	}


	return true;
}


uintptr_t HUD::LoadImage( const std::string& path )
{
	uintptr_t ret = 0;
	mGLContext->runOnGLThread( [this, &ret, path]() {
		ret = mRendererHUD->LoadImage( path );
	}, true);
	return ret;
}


void HUD::ShowImage( int32_t x, int32_t y, uint32_t w, uint32_t h, const uintptr_t img )
{
	mImagesMutex.lock();
	if ( mImages.find(img) == mImages.end() ) {
		Image struc = { img, x, y, w, h };
		mImages.insert( std::make_pair( img, struc ) );
	}
	mImagesMutex.unlock();
}


void HUD::HideImage( const uintptr_t img )
{
	mImagesMutex.lock();
	if ( mImages.find(img) != mImages.end() ) {
		mImages.erase( img );
	}
	mImagesMutex.unlock();
}




#endif // ( BUILD_HUD == 1 )
