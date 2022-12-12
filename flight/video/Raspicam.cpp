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

#ifdef BOARD_rpi

#include <unistd.h>
#include <dirent.h>
#include <netinet/in.h>
#include <Debug.h>
#include <Link.h>
#include <RawWifi.h>
#include <Socket.h>
#include <GPIO.h>
#include "Raspicam.h"
#include <Board.h>
#include "Recorder.h"
#include "GLContext.h"
#include <Main.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Broadcom.h>


Raspicam::ModelSettings::ModelSettings()
	: whiteBalance( "" )
{
	sensorMode = 0;
	width = 1280;
	height = 720;
	fps = 60;
	exposure = 0;
	iso = 0;
	shutterSpeed = 0;
	sharpness = 50;
	brightness = 55;
	contrast = 0;
	saturation = 8;
	vflip = false;
	hflip = false;
	whiteBalance = "auto";
	stabilisation = true;
	night_fps = 0;
	night_iso = 0;
	night_brightness = 80;
	night_contrast = 100;
	night_saturation = -25;
	fDebug();
}


Raspicam::Raspicam()
	: ::Camera()
	, mLink( nullptr )
	, mDirectMode( false )
	, mSettings( nullptr )
	, mWidth( 0 )
	, mHeight( 0 )
	, mISO( 0 )
	, mShutterSpeed( 0 )
	, mLiveFrameCounter( 0 )
	, mLedTick( 0 )
	, mHeadersTick( 0 )
	, mLedState( true )
	, mNightMode( false )
	, mPaused( false )
	, mWhiteBalance( CAM_INTF::Camera::WhiteBalControlAuto )
	, mExposureMode( CAM_INTF::Camera::ExposureControlAuto )
	, mWhiteBalanceLock( "" )
	, mRecording( false )
	, mSplitter( nullptr )
	, mEncoderRecord( nullptr )
	, mRecordThread( nullptr )
	, mTakingPicture( false )
	, mLastPictureID( 0 )
	, mRecordStream( nullptr )
	, mRecorderTrackId( 0 )
{
	fDebug();
	CAM_INTF::Component::setDebugOutputCallback( &Raspicam::DebugOutput );
}


Raspicam::~Raspicam()
{
}


void Raspicam::Start()
{
	mHandle = new CAM_INTF::Camera( 0, 0, 0, true, 0, true );
/*
	if ( !mHandle->handle() ) {
		Board::defectivePeripherals()["Camera"] = true;
		return;
	}
*/

	CAM_INTF::Camera::Infos infos = mHandle->getInfos();
	if ( infos.name.find("ov5647") != infos.name.npos ) {
		mName = "v1";
		mSettings = &mSettingsV1;
	}
	if ( infos.name.find("imx219") != infos.name.npos ) {
		mName = "v2";
		mSettings = &mSettingsV2;
	}
	if ( infos.name.find("testc") != infos.name.npos ) {
		mName = "hq";
		mSettings = &mSettingsHQ;
	}
	if ( !mSettings ) {
		gDebug() << "WARNING : Unknown Raspberry Pi Camera model \"" << infos.name << "\" !";
		mSettings = &mSettingsV2;
	}

	gDebug() << "Current camera " <<  mName << " ModelSettings :";
	gDebug() << "    sensor_mode : " << mSettings->sensorMode;
	gDebug() << "    width : " << mSettings->width;
	gDebug() << "    height : " << mSettings->height;
	gDebug() << "    fps : " << mSettings->fps;
	gDebug() << "    sharpness : " << mSettings->sharpness;
	gDebug() << "    brightness : " << mSettings->brightness;
	gDebug() << "    contrast : " << mSettings->contrast;
	gDebug() << "    saturation : " << mSettings->saturation;
	gDebug() << "    vflip : " << mSettings->vflip;
	gDebug() << "    hflip : " << mSettings->hflip;
	gDebug() << "    whiteBalance : " << mSettings->whiteBalance;
	gDebug() << "    stabilisation : " << mSettings->stabilisation;
	gDebug() << "    night_fps : " << mSettings->night_fps;
	gDebug() << "    night_iso : " << mSettings->night_iso;
	gDebug() << "    night_brightness : " << mSettings->night_brightness;
	gDebug() << "    night_contrast : " << mSettings->night_contrast;
	gDebug() << "    night_saturation : " << mSettings->night_saturation;

	mWidth = mSettings->width;
	mHeight = mSettings->height;
	// TEST : record everytime
	// TODO : remove this
	mRecording = true;


	mHandle->setSensorMode( mSettings->sensorMode );

	if ( mDisableLensShading ) {
		mHandle->disableLensShading();
	}
/*
	if ( mConfig->ArrayLength( conf_obj + ".lens_shading" ) > 0 ) {
		if ( mConfig->Number( conf_obj + ".lens_shading.r.base", -10.0f ) != -10.0f ) {
			auto convert = [](float f) {
				bool neg = ( f < 0.0f );
				float ipart;
				return ((int)f * 32) + ( neg ? -1 : 1 ) * (int)( modf( neg ? -f : f, &ipart ) * 32.0f );
			};
			mLensShaderR.base = convert( mConfig->Number( conf_obj + ".lens_shading.r.base", 2.0f ) );
			mLensShaderR.radius =  mConfig->Integer( conf_obj + ".lens_shading.r.radius", 0 );
			mLensShaderR.strength = convert( mConfig->Number( conf_obj + ".lens_shading.r.strength", 0.0f ) );
			mLensShaderG.base = convert( mConfig->Number( conf_obj + ".lens_shading.g.base", 2.0f ) );
			mLensShaderG.radius =  mConfig->Integer( conf_obj + ".lens_shading.g.radius", 0 );
			mLensShaderG.strength = convert( mConfig->Number( conf_obj + ".lens_shading.g.strength", 0.0f ) );
			mLensShaderB.base = convert( mConfig->Number( conf_obj + ".lens_shading.b.base", 2.0f ) );
			mLensShaderB.radius =  mConfig->Integer( conf_obj + ".lens_shading.b.radius", 0 );
			mLensShaderB.strength = convert( mConfig->Number( conf_obj + ".lens_shading.b.strength", 0.0f ) );
			setLensShader_internal( mLensShaderR, mLensShaderG, mLensShaderB );
		} else {
			vector< int32_t > gr, gb;
			auto r = mConfig->IntegerArray( conf_obj + ".lens_shading.r" );
			auto b = mConfig->IntegerArray( conf_obj + ".lens_shading.b" );
			if ( mConfig->ArrayLength( conf_obj + ".lens_shading.g" ) > 0 ) {
				gr = mConfig->IntegerArray( conf_obj + ".lens_shading.g" );
				gb = gr;
			} else {
				gr = mConfig->IntegerArray( conf_obj + ".lens_shading.gr" );
				gb = mConfig->IntegerArray( conf_obj + ".lens_shading.gb" );
			}
			uint8_t* grid = new uint8_t[52 * 39 * 4];
			for ( int32_t y = 0; y < 39; y++ ) {
				for ( int32_t x = 0; x < 52; x++ ) {
					grid[ 52 * 39 * 0 + y * 52 + x ] = r.data()[y * 52 + x];
					grid[ 52 * 39 * 1 + y * 52 + x ] = gr.data()[y * 52 + x];
					grid[ 52 * 39 * 2 + y * 52 + x ] = gb.data()[y * 52 + x];
					grid[ 52 * 39 * 3 + y * 52 + x ] = b.data()[y * 52 + x];
				}
			}
			mHandle->setLensShadingGrid( 64, 52, 39, 3, grid );
		}
	}
*/
	mHandle->setResolution( mWidth, mHeight, 71 );
	mHandle->setMirror( mSettings->hflip, mSettings->vflip );
	mHandle->setWhiteBalanceControl( CAM_INTF::Camera::WhiteBalControlAuto );
	mHandle->setExposureControl( CAM_INTF::Camera::ExposureControlAuto );
	mHandle->setExposureValue( CAM_INTF::Camera::ExposureMeteringModeSpot, mSettings->exposure, mSettings->iso, mSettings->shutterSpeed );
	mHandle->setSharpness( mSettings->sharpness );
	mHandle->setFramerate( mSettings->fps );
	mHandle->setBrightness( mSettings->brightness );
	mHandle->setSaturation( mSettings->saturation );
	mHandle->setContrast( mSettings->contrast );
	mHandle->setFrameStabilisation( mSettings->stabilisation );

// 	mHandle->setImageFilter( mHandle->ImageFilterDeInterlaceFast );

	string whitebal = mSettings->whiteBalance;
	CAM_INTF::Camera::WhiteBalControl wbcontrol = CAM_INTF::Camera::WhiteBalControlAuto;
	if ( whitebal == "off" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlOff;
	} else if ( whitebal == "sunlight" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlSunLight;
	} else if ( whitebal == "cloudy" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlCloudy;
	} else if ( whitebal == "shade" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlShade;
	} else if ( whitebal == "tungsten" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlTungsten;
	} else if ( whitebal == "fluorescent" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlFluorescent;
	} else if ( whitebal == "incandescent" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlIncandescent;
	} else if ( whitebal == "flash" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlFlash;
	} else if ( whitebal == "horizon" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlHorizon;
	} else if ( whitebal == "greyworld" or whitebal == "grayworld" ) {
		wbcontrol = CAM_INTF::Camera::WhiteBalControlGreyWorld;
	}
	mHandle->setWhiteBalanceControl( wbcontrol );

	// mRender = new CAM_INTF::VideoRender();
	mRender = new CAM_INTF::EGLRender();
	gDebug() << "mWidth : " << mWidth;
	gDebug() << "mHeight : " << mHeight;
	int32_t glFrame = GLContext::instance()->addLayer( 2048, 2048, GL_RGBA, -1 );
	gDebug() << "glFrame : " << glFrame;
	EGLImageKHR mEGLVideoImage = GLContext::instance()->eglImage( glFrame );
	gDebug() << "mEGLVideoImage : " << mEGLVideoImage;
	mRender->setEGLImage( mEGLVideoImage );
	mHandle->SetupTunnelPreview( mRender );

	mImageEncoder = new CAM_INTF::ImageEncode( CAM_INTF::ImageEncode::CodingJPEG, true );
	mHandle->SetupTunnelImage( mImageEncoder );
	mTakePictureThread = new HookThread<Raspicam>( "cam_still", this, &Raspicam::TakePictureThreadRun );

	mEncoder = new CAM_INTF::VideoEncode( mRecordBitRateKbps, CAM_INTF::VideoEncode::CodingAVC, not mDirectMode, false );
/*
	if ( mBetterRecording ) {
		mSplitter = new CAM_INTF::VideoSplitter( true );
// 		mEncoderRecord = new CAM_INTF::VideoEncode( mConfig->Integer( mConfigObject + ".record_kbps", 16384 ), CAM_INTF::VideoEncode::CodingAVC, true );
		mHandle->SetupTunnelVideo( mSplitter );
		mSplitter->SetupTunnel( mEncoder );
// 		mSplitter->SetupTunnel( mEncoderRecord );
		mEncoder->SetState( CAM_INTF::Component::StateIdle );
// 		mEncoderRecord->SetState( CAM_INTF::Component::StateIdle );
		mEncoder->AllocateOutputBuffer();
// 		mEncoderRecord->AllocateOutputBuffer();
		mSplitter->SetState( CAM_INTF::Component::StateIdle );
	} else*/ {
		mHandle->SetupTunnelVideo( mEncoder );
		mEncoder->SetState( CAM_INTF::Component::StateIdle );
		mEncoder->AllocateOutputBuffer();
	}
	mHandle->SetState( CAM_INTF::Component::StateIdle );
	mRender->SetState( CAM_INTF::Component::StateIdle );

	mHandle->SetState( CAM_INTF::Component::StateExecuting );

	mHandle->SetState( CAM_INTF::Component::StateExecuting );
	mRender->SetState( CAM_INTF::Component::StateExecuting );
	if ( mSplitter ) {
		mSplitter->SetState( CAM_INTF::Component::StateExecuting );
// 		mEncoderRecord->SetState( CAM_INTF::Component::StateExecuting );
	}
	mEncoder->SetState( CAM_INTF::Component::StateExecuting );
	mImageEncoder->SetState( CAM_INTF::Component::StateExecuting );
	mHandle->SetCapturing( true );

	mLiveTicks = 0;
	mRecordTicks = 0;
	mRecordFrameData = nullptr;
	mRecordFrameDataSize = 0;
	mRecordFrameSize = 0;
	mLiveBuffer = new uint8_t[2*1024*1024];

	if ( Main::instance()->recorder() ) {
		mRecorderTrackId = Main::instance()->recorder()->AddVideoTrack( mWidth, mHeight, mHandle->framerate(), "h264" );
	}

	mLiveThread = new HookThread<Raspicam>( "cam_live", this, &Raspicam::LiveThreadRun );
	mLiveThread->Start();
	mLiveThread->setPriority( 99, 3 );

	mRecordThread = new HookThread<Raspicam>( "cam_record", this, &Raspicam::RecordThreadRun );
// 	mRecordThread->Start();

	mTakePictureThread->Start();
/*
	// TEST
	mWhiteBalance = WhiteBalControlOff;
	CAM_INTF::Camera::setWhiteBalanceControl( mWhiteBalance );

	OMX_CONFIG_CUSTOMAWBGAINSTYPE awb;
	OMX_INIT_STRUCTURE( awb );
	awb.xGainR = 65535 * 1.2;
	awb.xGainB = 65535 * 1.7;
	CAM_INTF::Camera::SetConfig( OMX_IndexConfigCustomAwbGains, &awb );
	printf( "locked (R:%.2f, B:%.2f)\n", ((float)awb.xGainR) / 65535.0f, ((float)awb.xGainB) / 65535.0f );
	// TEST
*/
}


void Raspicam::DebugOutput( int level, const string fmt, ... )
{
	(void)level;
	va_list opt;
	char buff[8192] = "";

	va_start( opt, fmt );
	vsnprintf( buff, (size_t) sizeof(buff), fmt.c_str(), opt );

	gDebug() << buff;
}


const uint32_t Raspicam::framerate()
{
	return mHandle->framerate();
}


const uint32_t Raspicam::brightness()
{
	return mHandle->brightness();
}


const int32_t Raspicam::contrast()
{
	return mHandle->contrast();
}


const int32_t Raspicam::saturation()
{
	return mHandle->saturation();
}


const int32_t Raspicam::ISO()
{
	return mISO;
}


const uint32_t Raspicam::shutterSpeed()
{
	return mShutterSpeed;
}


const bool Raspicam::nightMode()
{
	return mNightMode;
}


const string Raspicam::whiteBalance()
{
	if ( mWhiteBalanceLock != "" ) {
		return mWhiteBalanceLock;
	}

	switch( mWhiteBalance ) {
		case CAM_INTF::Camera::WhiteBalControlOff:
			return "off";
			break;
		case CAM_INTF::Camera::WhiteBalControlAuto:
			return "auto";
			break;
		case CAM_INTF::Camera::WhiteBalControlSunLight:
			return "sunlight";
			break;
		case CAM_INTF::Camera::WhiteBalControlCloudy:
			return "cloudy";
			break;
		case CAM_INTF::Camera::WhiteBalControlShade:
			return "shade";
			break;
		case CAM_INTF::Camera::WhiteBalControlTungsten:
			return "tungsten";
			break;
		case CAM_INTF::Camera::WhiteBalControlFluorescent:
			return "fluorescent";
			break;
		case CAM_INTF::Camera::WhiteBalControlIncandescent:
			return "incandescent";
			break;
		case CAM_INTF::Camera::WhiteBalControlFlash:
			return "flash";
			break;
		case CAM_INTF::Camera::WhiteBalControlHorizon:
			return "horizon";
			break;
		case CAM_INTF::Camera::WhiteBalControlGreyWorld:
			return "greyworld";
			break;
	}
	return "none";
}


const string Raspicam::exposureMode()
{
	switch( mExposureMode ) {
		case CAM_INTF::Camera::ExposureControlOff:
			return "off";
			break;
		case CAM_INTF::Camera::ExposureControlAuto:
			return "auto";
			break;
		case CAM_INTF::Camera::ExposureControlNight:
			return "night";
			break;
		case CAM_INTF::Camera::ExposureControlBackLight:
			return "backlight";
			break;
		case CAM_INTF::Camera::ExposureControlSpotLight:
			return "spotlight";
			break;
		case CAM_INTF::Camera::ExposureControlSports:
			return "sports";
			break;
		case CAM_INTF::Camera::ExposureControlSnow:
			return "snow";
			break;
		case CAM_INTF::Camera::ExposureControlBeach:
			return "beach";
			break;
		case CAM_INTF::Camera::ExposureControlLargeAperture:
			return "largeaperture";
			break;
		case CAM_INTF::Camera::ExposureControlSmallAperture:
			return "smallaperture";
			break;
		case CAM_INTF::Camera::ExposureControlVeryLong:
			return "verylong";
			break;
		case CAM_INTF::Camera::ExposureControlFixedFps:
			return "fixedfps";
			break;
		case CAM_INTF::Camera::ExposureControlNightWithPreview:
			return "nightpreview";
			break;
		case CAM_INTF::Camera::ExposureControlAntishake:
			return "antishake";
			break;
		case CAM_INTF::Camera::ExposureControlFireworks:
			return "fireworks";
			break;
		default:
			break;
	}
	return "none";
}


uint32_t Raspicam::getLastPictureID()
{
	return mLastPictureID;
}


const bool Raspicam::recording()
{
	return ( mRecording and not mPaused );
}


const string Raspicam::recordFilename()
{
	return mRecordFilename;
}


void Raspicam::setBrightness( uint32_t value )
{
	mHandle->setBrightness( value );
}


void Raspicam::setContrast( int32_t value )
{
	mHandle->setContrast( value );
}


void Raspicam::setSaturation( int32_t value )
{
	mHandle->setSaturation( value );
}


void Raspicam::setISO( int32_t value )
{
	mISO = value;
	mHandle->setExposureValue( mHandle->ExposureMeteringModeMatrix, 0, mISO, mShutterSpeed );
}


void Raspicam::setNightMode( bool night_mode )
{
	if ( mNightMode != night_mode ) {
		mNightMode = night_mode;
		if ( mNightMode ) {
			mHandle->setFramerate( mSettings->night_fps > 0 ? mSettings->night_fps : mSettings->fps );
			mHandle->setExposureValue( CAM_INTF::Camera::ExposureMeteringModeMatrix, 0, ( mISO = mSettings->night_iso ), mShutterSpeed );
			mHandle->setBrightness( mSettings->night_brightness );
			mHandle->setContrast( mSettings->night_contrast );
			mHandle->setSaturation( mSettings->night_saturation );
		} else {
			mHandle->setFramerate( mSettings->fps );
			mHandle->setExposureValue( CAM_INTF::Camera::ExposureMeteringModeMatrix, 0, ( mISO = mSettings->iso ), mShutterSpeed );
			mHandle->setBrightness( mSettings->brightness );
			mHandle->setContrast( mSettings->contrast );
			mHandle->setSaturation( mSettings->saturation );
		}
	}
}


string Raspicam::switchWhiteBalance()
{
	mWhiteBalanceLock = "";
	mWhiteBalance = (CAM_INTF::Camera::WhiteBalControl)( ( mWhiteBalance + 1 ) % ( CAM_INTF::Camera::WhiteBalControlGreyWorld + 1 ) );
	mHandle->setWhiteBalanceControl( mWhiteBalance );
	return whiteBalance();
}


string Raspicam::lockWhiteBalance()
{
	// TODO : move it to OpenMaxIL++
#if ( CAM_USE_MMAL == 1 )
	return "";
#else
	OMX_CONFIG_CAMERASETTINGSTYPE settings;
	CAM_INTF::Component::OMX_INIT_STRUCTURE( settings );
	settings.nPortIndex = 71;
	mHandle->GetConfig( OMX_IndexConfigCameraSettings, &settings );

	mWhiteBalance = CAM_INTF::Camera::WhiteBalControlOff;
	mHandle->setWhiteBalanceControl( mWhiteBalance );

	OMX_CONFIG_CUSTOMAWBGAINSTYPE awb;
	CAM_INTF::Component::OMX_INIT_STRUCTURE( awb );
	awb.xGainR = settings.nRedGain << 8;
	awb.xGainB = settings.nBlueGain << 8;
	mHandle->SetConfig( OMX_IndexConfigCustomAwbGains, &awb );

	char ret[64];
	sprintf( ret, "locked (R:%.2f, B:%.2f)", ((float)awb.xGainR) / 65535.0f, ((float)awb.xGainB) / 65535.0f );
	mWhiteBalanceLock = string(ret);
	return mWhiteBalanceLock;
#endif
}


string Raspicam::switchExposureMode()
{
	mExposureMode = (CAM_INTF::Camera::ExposureControl)( mExposureMode + 1 );
	if ( mExposureMode == CAM_INTF::Camera::ExposureControlSmallAperture + 1 ) {
		mExposureMode = CAM_INTF::Camera::ExposureControlVeryLong;
	}
	if ( mExposureMode > CAM_INTF::Camera::ExposureControlFireworks ) {
		mExposureMode = CAM_INTF::Camera::ExposureControlOff;
	}

	mHandle->setExposureControl( mExposureMode );
	return exposureMode();
}


void Raspicam::setShutterSpeed( uint32_t value )
{
	mShutterSpeed = value;
	mHandle->setExposureValue( CAM_INTF::Camera::ExposureMeteringModeMatrix, 0, mISO, mShutterSpeed );
}


void Raspicam::setLensShader_internal( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B )
{
	uint8_t* grid = new uint8_t[52 * 39 * 4];
	for ( int32_t y = 0; y < 39; y++ ) {
		for ( int32_t x = 0; x < 52; x++ ) {
			int32_t r = R.base;
			int32_t g = G.base;
			int32_t b = B.base;
			int32_t dist = sqrt( ( x - 52 / 2 ) * ( x - 52 / 2 ) + ( y - 39 / 2 ) * ( y - 39 / 2 ) );
			if ( R.radius > 0 ) {
				float dot_r = (float)max( 0, R.radius - dist ) / (float)R.radius;
				r = max( 0, min( 255, r + (int32_t)( dot_r * R.strength ) ) );
			}
			if ( G.radius > 0 ) {
				float dot_g = (float)max( 0, G.radius - dist ) / (float)G.radius;
				g = max( 0, min( 255, g + (int32_t)( dot_g * G.strength ) ) );
			}
			if ( B.radius > 0 ) {
				float dot_b = (float)max( 0, B.radius - dist ) / (float)B.radius;
				b = max( 0, min( 255, b + (int32_t)( dot_b * B.strength ) ) );
			}
			grid[ (x + y * 52) + ( 52 * 39 ) * 0 ] = r;
			grid[ (x + y * 52) + ( 52 * 39 ) * 1 ] = g;
			grid[ (x + y * 52) + ( 52 * 39 ) * 2 ] = g;
			grid[ (x + y * 52) + ( 52 * 39 ) * 3 ] = b;
		}
	}
	mHandle->setLensShadingGrid( 64, 52, 39, 3, grid );
}


void Raspicam::setLensShader( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B )
{
	mLensShaderR = R;
	mLensShaderG = G;
	mLensShaderB = B;

	mHandle->SetCapturing( false );
	mHandle->SetState( CAM_INTF::Component::StateIdle );
	mHandle->SetState( CAM_INTF::Component::StateLoaded );
	setLensShader_internal( R, G, B );
	mHandle->SetState( CAM_INTF::Component::StateIdle );
	mHandle->SetState( CAM_INTF::Component::StateExecuting );
	mHandle->SetCapturing( true );
}


void Raspicam::getLensShader( LensShaderColor* r, LensShaderColor* g, LensShaderColor* b )
{
	*r = mLensShaderR;
	*g = mLensShaderG;
	*b = mLensShaderB;
	// probably using a full table, or not setting lens shading at all
	if ( r->base == 0 and g->base == 0 and b->base == 0 ) {
		r->base = 96;
		g->base = 96;
		b->base = 96;
	}
}


void Raspicam::Pause()
{
	mPaused = true;
}


void Raspicam::Resume()
{
	mPaused = false;
}


void Raspicam::StartRecording()
{
	if ( not mRecording ) {
		Recorder* recorder = Main::instance()->recorder();
		if ( recorder ) {
			recorder->Start();
		}
		mRecording = true;
	}
}


void Raspicam::StopRecording()
{
	if ( mRecording ) {
		mRecording = false;
		if ( mRecordStream ) {
// 			delete mRecordStream;
			fclose( mRecordStream );
			mRecordStream = nullptr;
		}
		Recorder* recorder = Main::instance()->recorder();
		if ( recorder ) {
			recorder->Stop();
		}
	}
}


void Raspicam::TakePicture()
{
	if ( mTakingPicture ) {
		return;
	}
	unique_lock<mutex> locker( mTakePictureMutex );
	mTakingPicture = true;
	mTakePictureCond.notify_all();
}


bool Raspicam::TakePictureThreadRun()
{
	unique_lock<mutex> locker( mTakePictureMutex );
	cv_status timeout = mTakePictureCond.wait_for( locker, chrono::milliseconds( 1 * 1000 ) );
	locker.unlock();
	if ( timeout == cv_status::timeout ) {
		return true;
	}

	uint8_t* buf = new uint8_t[8*1024*1024];
	uint32_t buflen = 0;
	bool end_of_frame = false;

	mHandle->SetCapturing( true, 72 );
	do {
		buflen += mImageEncoder->getOutputData( &buf[buflen], &end_of_frame, true );
	} while ( end_of_frame == false );
	mHandle->SetCapturing( true );

	char filename[256];
	uint32_t fileid = 0;
	DIR* dir;
	struct dirent* ent;
	if ( ( dir = opendir( "/var/PHOTO" ) ) != nullptr ) {
		while ( ( ent = readdir( dir ) ) != nullptr ) {
			string file = string( ent->d_name );
			uint32_t id = atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
			if ( id >= fileid ) {
				fileid = id + 1;
			}
		}
		closedir( dir );
	}
	sprintf( filename, "/var/PHOTO/photo_%06u.jpg", fileid );
	FILE* fp = fopen( filename, "wb" );
	fwrite( buf, 1, buflen, fp );
	fclose( fp );

	gDebug() << "Picture taken !";
	delete[] buf;
	mLastPictureID = fileid;
	mTakingPicture = false;
	return true;
}


bool Raspicam::LiveThreadRun()
{
	if ( mRecording and Board::GetTicks() - mLedTick >= 1000 * 500 ) {
		GPIO::Write( 32, ( mLedState = !mLedState ) );
		mLedTick = Board::GetTicks();
	}
	/*
	mEncoder->dataAvailable( true );
	mEncoder->fillBuffer();
	uint32_t datalen = mEncoder->bufferLength();
	pData = mEncoder->buffer();
	*/
	uint32_t datalen = mEncoder->getOutputData( mLiveBuffer, true );

	if ( not mDirectMode and not mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Raspicam connected !";
			mLink->setBlocking( false );
			mLiveFrameCounter = 0;
		} else {
			return true;
		}
	}

	if ( mPaused ) {
		usleep( 1000 * 50 );
		return true;
	}

	if ( (int32_t)datalen > 0 ) {
		mLiveFrameCounter++;
		if ( not mDirectMode ) {
			if ( dynamic_cast<Socket*>(mLink) ) {
				uint32_t dummy = 0;
				mLink->Read( &dummy, sizeof(dummy), 0 ); // Dummy Read to get sockaddr_t from client
			}
			LiveSend( (char*)mLiveBuffer, datalen );
		}
		if ( mRecording and not mBetterRecording ) {
			RecordWrite( (char*)mLiveBuffer, datalen );
		}
	}

	// Send video headers every 5 seconds
	if ( Board::GetTicks() - mHeadersTick >= 1000 * 1000 * 5 ) {
		mHeadersTick = Board::GetTicks();
		const map< uint32_t, uint8_t* > headers = mEncoder->headers();
		if ( headers.size() > 0 ) {
			for ( auto hdr : headers ) {
				if ( not mDirectMode ) {
					LiveSend( (char*)hdr.second, hdr.first );
				}
				if ( mRecording and not mBetterRecording ) {
					RecordWrite( (char*)hdr.second, hdr.first );
				}
			}
		}
	}

	return true;
}


bool Raspicam::RecordThreadRun()
{
	if ( not mRecording ) {
		usleep( 1000 * 100 );
		return true;
	}

	if ( mPaused ) {
		usleep( 1000 * 50 );
		return true;
	}

	mRecordStreamMutex.lock();
	if ( mRecordStream ) {
		while ( mRecordStreamQueue.size() > 0 ) {
			pair< uint8_t*, uint32_t > frame = mRecordStreamQueue.front();
			mRecordStreamQueue.pop_front();
			mRecordStreamMutex.unlock();
			if ( fwrite( frame.first, 1, frame.second, mRecordStream ) != frame.second ) {
				if ( errno == ENOSPC ) {
					Board::setDiskFull();
				}
			}
			delete frame.first;
			if ( mRecordSyncCounter == 0 ) {
				if ( fflush( mRecordStream ) < 0 or fsync( fileno( mRecordStream ) ) < 0 ) {
					if ( errno == ENOSPC ) {
						Board::setDiskFull();
					}
				}
			}
			mRecordSyncCounter = ( mRecordSyncCounter + 1 ) % 10; // sync on disk every 10 frames (up to 10*1/FPS seconds)
			usleep( 1000 );
			mRecordStreamMutex.lock();
		}
	}
	mRecordStreamMutex.unlock();

	if ( mEncoderRecord ) {
		uint8_t data[65536*2];
		uint32_t datalen = mEncoderRecord->getOutputData( data );
		if ( (int32_t)datalen > 0 ) {
			RecordWrite( (char*)data, datalen );
		}
	}

	return true;
}


int Raspicam::LiveSend( char* data, int datalen )
{
	if ( datalen <= 0 ) {
		return datalen;
	}

	int err = mLink->Write( (uint8_t*)data, datalen, false, 0 );

	if ( err < 0 ) {
		gDebug() << "Link->Write() error : " << strerror(errno) << " (" << errno << ")";
		return -1;
	}
	return 0;
}


int Raspicam::RecordWrite( char* data, int datalen, int64_t pts, bool audio )
{
//	return 0;
	Recorder* recorder = Main::instance()->recorder();
	if ( recorder ) {
		recorder->WriteSample( mRecorderTrackId, Board::GetTicks(), data, datalen );
		return datalen;
	}

	int ret = 0;

	if ( !mRecordStream ) {
		mEncoder->RequestIFrame();
		char filename[256];
		uint32_t fileid = 0;
		DIR* dir;
		struct dirent* ent;
		if ( ( dir = opendir( "/var/VIDEO" ) ) != nullptr ) {
			while ( ( ent = readdir( dir ) ) != nullptr ) {
				string file = string( ent->d_name );
				uint32_t id = atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
				if ( id >= fileid ) {
					fileid = id + 1;
				}
			}
			closedir( dir );
		}
		sprintf( filename, "/var/VIDEO/video_%02dfps_%06u.h264", mHandle->framerate(), fileid );
		mRecordFilename = string( filename );
		FILE* stream = fopen( filename, "wb" );
		const map< uint32_t, uint8_t* > headers = mEncoder->headers();
		if ( headers.size() > 0 ) {
			for ( auto hdr : headers ) {
				fwrite( hdr.second, 1, hdr.first, stream );
			}
		}
		mRecordStream = stream;
	}

	pair< uint8_t*, uint32_t > frame;
	frame.first = new uint8_t[datalen];
	frame.second = datalen;
	memcpy( frame.first, data, datalen );
	mRecordStreamMutex.lock();
	mRecordStreamQueue.emplace_back( frame );
	mRecordStreamMutex.unlock();

	return ret;
}


uint32_t* Raspicam::getFileSnapshot( const string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp )
{
	uint32_t* data = nullptr;
	*width = 0;
	*height = 0;
	*bpp = 0;

	ifstream file( filename, ios_base::in | ios_base::binary );
	if ( file.is_open() ) {
		CAM_INTF::VideoDecode* decoder = new CAM_INTF::VideoDecode( 60, CAM_INTF::VideoDecode::CodingAVC, true );
		decoder->setRGB565Mode( true );
		uint32_t buf[2048];
		uint32_t frame = 0;
		decoder->SetState( CAM_INTF::Component::StateExecuting );
		while ( not file.eof() and ( not decoder->valid() or frame < 32 ) ) {
			streamsize sz = file.readsome( (char*)buf, sizeof(buf) );
			decoder->fillInput( (uint8_t*)buf, sz );
			if ( decoder->valid() ) {
				frame++;
			}
		}
		*width = decoder->width();
		*height = decoder->height();
		*bpp = 16;
		data = (uint32_t*)malloc( 16 * decoder->width() * decoder->height() );
		decoder->getOutputData( (uint8_t*)data, false );
		decoder->SetState( CAM_INTF::Component::StateIdle );
		delete decoder;
	}

	return data;
}

#endif // BOARD_rpi
