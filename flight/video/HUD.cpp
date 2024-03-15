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
#include <Sensor.h>


HUD::HUD()
	: Thread( "HUD" )
	, mGLContext( nullptr )
	, mRendererHUD( nullptr )
	, mNightMode( false )
	, mHUDFramerate( 30 )
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
	, mReady( false )
{

	mGLContext = GLContext::instance();
	Start();
}


HUD::~HUD()
{
}


bool HUD::run()
{
	if ( not mReady ) {
		mReady = true;
		setFrequency( mHUDFramerate );

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
				mRendererHUD->RenderImage( mFrameLeft + img.x, mFrameTop + img.y, img.width, img.height, img.img );
			}
			mImagesMutex.unlock();

			if ( false ) {
				uint32_t time = Board::GetTicks() / 1000;
				uint32_t minuts = time / ( 1000 * 60 );
				uint32_t seconds = ( time / 1000 ) % 60;
				uint32_t ms = time % 1000;
				char txt[256];
				sprintf( txt, "%02d:%02d:%03d", minuts, seconds, ms );
				mRendererHUD->RenderText( 200, 200, txt, 0xFFFFFFFF, 4.0f, RendererHUD::TextAlignment::CENTER );
			}
		});
	}

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
		mAccelerationAccum = ( mAccelerationAccum * 0.995f + imu->acceleration().length() * 0.005f );
		dronestats.acceleration = mAccelerationAccum;
		dronestats.rpy = imu->RPY();
		if ( Sensor::GPSes().size() > 0 ) {
			dronestats.gpsLocation = imu->gpsLocation();
			dronestats.gpsSpeed = imu->gpsSpeed();
			dronestats.gpsSatellitesSeen = imu->gpsSatellitesSeen();
			dronestats.gpsSatellitesUsed = imu->gpsSatellitesUsed();
		} else {
			dronestats.gpsSpeed = NAN;
		}
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
		mVideoStats = VideoStats(
			(int)camera->width(),
			(int)camera->height(),
			(int)camera->framerate(),
			0,
			camera->whiteBalance()
		);
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


void HUD::AddLayer( const std::function<void()>& func )
{
	mGLContext->addLayer( func );
}


void HUD::PrintText( int32_t x, int32_t y, const std::string& text, uint32_t color, TextAlignment halign, TextAlignment valign )
{
	mRendererHUD->RenderText( x, y, text, color, 0.75f, (RendererHUD::TextAlignment)halign, (RendererHUD::TextAlignment)valign );
}


#endif // ( BUILD_HUD == 1 )
