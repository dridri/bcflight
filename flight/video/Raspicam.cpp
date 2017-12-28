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
#include <GPIO.h>
#include "Raspicam.h"
#include <Board.h>
#include <Recorder.h>
#include <Main.h>
#include "../../external/OpenMaxIL++/include/VideoDecode.h"
#include <IL/OMX_Index.h>
#include <IL/OMX_Broadcom.h>


Raspicam::Raspicam( Config* config, const std::string& conf_obj )
	: ::Camera()
	, IL::Camera( config->integer( conf_obj + ".width", 1280 ), config->integer( conf_obj + ".height", 720 ), 0, true, config->integer( conf_obj + ".sensor_mode", 0 ), true )
	, mConfig( config )
	, mConfigObject( conf_obj )
	, mLink( nullptr )
	, mDirectMode( config->boolean( conf_obj + ".direct_mode", false ) )
	, mWidth( 0 )
	, mHeight( 0 )
	, mISO( mConfig->integer( mConfigObject + ".iso", 0 ) )
	, mLiveFrameCounter( 0 )
	, mLedTick( 0 )
	, mHeadersTick( 0 )
	, mLedState( true )
	, mNightMode( false )
	, mPaused( false )
	, mWhiteBalance( WhiteBalControlAuto )
	, mWhiteBalanceLock( "" )
	, mRecording( false )
	, mSplitter( nullptr )
	, mEncoderRecord( nullptr )
	, mRecordThread( nullptr )
	, mTakingPicture( false )
	, mRecordStream( nullptr )
	, mRecorderTrackId( 0 )
{
	fDebug( config, conf_obj );

	if ( not IL::Component::mHandle ) {
		Board::defectivePeripherals()["Camera"] = true;
		return;
	}

// 	setDebugOutputCallback( &Raspicam::DebugOutput );

	// TEST : record everytime
	// TODO : remove this
	mRecording = true;

	mBetterRecording = ( config->integer( conf_obj + ".record_kbps", config->integer( conf_obj + ".kbps", 1024 ) ) != config->integer( conf_obj + ".kbps", 1024 ) );
	gDebug() << "Camera splitted encoders : " << mBetterRecording << "\n";

	if ( mConfig->boolean( mConfigObject + ".disable_lens_shading", false ) ) {
		IL::Camera::disableLensShading();
	}

	if ( mConfig->ArrayLength( conf_obj + ".lens_shading" ) > 0 ) {
		if ( mConfig->number( conf_obj + ".lens_shading.r.base", -10.0f ) != -10.0f ) {
			auto convert = [](float f) {
				bool neg = ( f < 0.0f );
				float ipart;
				return ((int)f * 32) + ( neg ? -1 : 1 ) * (int)( std::modf( neg ? -f : f, &ipart ) * 32.0f );
			};
			mLensShaderR.base = convert( mConfig->number( conf_obj + ".lens_shading.r.base", 2.0f ) );
			mLensShaderR.radius =  mConfig->integer( conf_obj + ".lens_shading.r.radius", 0 );
			mLensShaderR.strength = convert( mConfig->number( conf_obj + ".lens_shading.r.strength", 0.0f ) );
			mLensShaderG.base = convert( mConfig->number( conf_obj + ".lens_shading.g.base", 2.0f ) );
			mLensShaderG.radius =  mConfig->integer( conf_obj + ".lens_shading.g.radius", 0 );
			mLensShaderG.strength = convert( mConfig->number( conf_obj + ".lens_shading.g.strength", 0.0f ) );
			mLensShaderB.base = convert( mConfig->number( conf_obj + ".lens_shading.b.base", 2.0f ) );
			mLensShaderB.radius =  mConfig->integer( conf_obj + ".lens_shading.b.radius", 0 );
			mLensShaderB.strength = convert( mConfig->number( conf_obj + ".lens_shading.b.strength", 0.0f ) );
			setLensShader_internal( mLensShaderR, mLensShaderG, mLensShaderB );
		} else {
			auto r = mConfig->integerArray( conf_obj + ".lens_shading.r" );
			auto g = mConfig->integerArray( conf_obj + ".lens_shading.g" );
			auto b = mConfig->integerArray( conf_obj + ".lens_shading.b" );
			uint8_t* grid = new uint8_t[52 * 39 * 4];
			memcpy( &grid[52 * 39 * 0], r.data(), 52 * 39 );
			memcpy( &grid[52 * 39 * 1], g.data(), 52 * 39 );
			memcpy( &grid[52 * 39 * 2], g.data(), 52 * 39 );
			memcpy( &grid[52 * 39 * 3], b.data(), 52 * 39 );
			IL::Camera::setLensShadingGrid( 64, 52, 39, grid );
		}
	}

	mWidth = config->integer( conf_obj + ".video_width", config->integer( conf_obj + ".width", 1280 ) );
	mHeight = config->integer( conf_obj + ".video_height", config->integer( conf_obj + ".height", 720 ) );
	IL::Camera::setResolution( mWidth, mHeight, 71 );
	IL::Camera::setMirror( mConfig->boolean( mConfigObject + ".hflip", false ), mConfig->boolean( mConfigObject + ".vflip", false ) );
	IL::Camera::setWhiteBalanceControl( IL::Camera::WhiteBalControlAuto );
	IL::Camera::setExposureControl( IL::Camera::ExposureControlAuto );
	IL::Camera::setExposureValue( mConfig->integer( mConfigObject + ".exposure", 0 ), mConfig->integer( mConfigObject + ".aperture", 2.8f ), mConfig->integer( mConfigObject + ".iso", 0 ), mConfig->integer( mConfigObject + ".shutter_speed", 0 ) );
	IL::Camera::setSharpness( mConfig->integer( mConfigObject + ".sharpness", 100 ) );
	IL::Camera::setFramerate( mConfig->integer( mConfigObject + ".fps", 60 ) );
	IL::Camera::setBrightness( mConfig->integer( mConfigObject + ".brightness", 55 ) );
	IL::Camera::setSaturation( mConfig->integer( mConfigObject + ".saturation", 8 ) );
	IL::Camera::setContrast( mConfig->integer( mConfigObject + ".contrast", 0 ) );
	IL::Camera::setFrameStabilisation( mConfig->boolean( mConfigObject + ".stabilisation", false ) );
	printf( "============+> FILTER A\n" );
// 	IL::Camera::setImageFilter( IL::Camera::ImageFilterDeInterlaceFast );
	printf( "============+> FILTER B\n" );

	mRender = new IL::VideoRender();
	IL::Camera::SetupTunnelPreview( mRender );

	mImageEncoder = new IL::ImageEncode( IL::ImageEncode::CodingJPEG, true );
	IL::Camera::SetupTunnelImage( mImageEncoder );
	mTakePictureThread = new HookThread<Raspicam>( "cam_still", this, &Raspicam::TakePictureThreadRun );

	mEncoder = new IL::VideoEncode( mConfig->integer( mConfigObject + ".kbps", 1024 ), IL::VideoEncode::CodingAVC, not mDirectMode, false );

	if ( mBetterRecording ) {
		mSplitter = new IL::VideoSplitter( true );
// 		mEncoderRecord = new IL::VideoEncode( mConfig->integer( mConfigObject + ".record_kbps", 16384 ), IL::VideoEncode::CodingAVC, true );
		IL::Camera::SetupTunnelVideo( mSplitter );
		mSplitter->SetupTunnel( mEncoder );
// 		mSplitter->SetupTunnel( mEncoderRecord );
		mEncoder->SetState( IL::Component::StateIdle );
// 		mEncoderRecord->SetState( IL::Component::StateIdle );
		mEncoder->AllocateOutputBuffer();
// 		mEncoderRecord->AllocateOutputBuffer();
		mSplitter->SetState( IL::Component::StateIdle );
	} else {
		IL::Camera::SetupTunnelVideo( mEncoder );
		mEncoder->SetState( IL::Component::StateIdle );
		mEncoder->AllocateOutputBuffer();
	}
	IL::Camera::SetState( IL::Component::StateIdle );
	mRender->SetState( IL::Component::StateIdle );

	IL::Camera::SetState( IL::Component::StateExecuting );

	IL::Camera::SetState( IL::Component::StateExecuting );
	mRender->SetState( IL::Component::StateExecuting );
	if ( mSplitter ) {
		mSplitter->SetState( IL::Component::StateExecuting );
// 		mEncoderRecord->SetState( IL::Component::StateExecuting );
	}
	mEncoder->SetState( IL::Component::StateExecuting );
	mImageEncoder->SetState( IL::Component::StateExecuting );
	IL::Camera::SetCapturing( true );

	mLiveTicks = 0;
	mRecordTicks = 0;
	mRecordFrameData = nullptr;
	mRecordFrameDataSize = 0;
	mRecordFrameSize = 0;
	mLiveBuffer = new uint8_t[2*1024*1024];

	if ( Main::instance()->recorder() ) {
		mRecorderTrackId = Main::instance()->recorder()->AddVideoTrack( mWidth, mHeight, IL::Camera::framerate(), "h264" );
	}

	mLink = Link::Create( config, conf_obj + ".link" );

	mLiveThread = new HookThread<Raspicam>( "cam_live", this, &Raspicam::LiveThreadRun );
	mLiveThread->Start();
// 	if ( not mDirectMode ) {
		mLiveThread->setPriority( 99, 3 );
// 	}

	mRecordThread = new HookThread<Raspicam>( "cam_record", this, &Raspicam::RecordThreadRun );
	mRecordThread->Start();

	mTakePictureThread->Start();
}


Raspicam::~Raspicam()
{
}


void Raspicam::DebugOutput( int level, const std::string fmt, ... )
{
	(void)level;
	va_list opt;
	char buff[8192] = "";

	va_start( opt, fmt );
	vsnprintf( buff, (size_t) sizeof(buff), fmt.c_str(), opt );

	Debug() << buff;
}


const uint32_t Raspicam::framerate()
{
	return IL::Camera::framerate();
}


const uint32_t Raspicam::brightness()
{
	return IL::Camera::brightness();
}


const int32_t Raspicam::contrast()
{
	return IL::Camera::contrast();
}


const int32_t Raspicam::saturation()
{
	return IL::Camera::saturation();
}


const int32_t Raspicam::ISO()
{
	return mISO;
}


const bool Raspicam::nightMode()
{
	return mNightMode;
}


const std::string Raspicam::whiteBalance()
{
	if ( mWhiteBalanceLock != "" ) {
		return mWhiteBalanceLock;
	}

	switch( mWhiteBalance ) {
		case WhiteBalControlOff:
			return "off";
			break;
		case WhiteBalControlAuto:
			return "auto";
			break;
		case WhiteBalControlSunLight:
			return "sunlight";
			break;
		case WhiteBalControlCloudy:
			return "cloudy";
			break;
		case WhiteBalControlShade:
			return "shade";
			break;
		case WhiteBalControlTungsten:
			return "tungsten";
			break;
		case WhiteBalControlFluorescent:
			return "fluorescent";
			break;
		case WhiteBalControlIncandescent:
			return "incandescent";
			break;
		case WhiteBalControlFlash:
			return "flash";
			break;
		case WhiteBalControlHorizon:
			return "horizon";
			break;
	}
	return "none";
}


const bool Raspicam::recording()
{
	return ( mRecording and not mPaused );
}


const std::string Raspicam::recordFilename()
{
	return mRecordFilename;
}


void Raspicam::setBrightness( uint32_t value )
{
	IL::Camera::setBrightness( value );
}


void Raspicam::setContrast( int32_t value )
{
	IL::Camera::setContrast( value );
}


void Raspicam::setSaturation( int32_t value )
{
	IL::Camera::setSaturation( value );
}


void Raspicam::setISO( int32_t value )
{
	mISO = value;
	IL::Camera::setExposureValue( 0, 2.8f, mISO, 0 );
}


void Raspicam::setNightMode( bool night_mode )
{
	if ( mNightMode != night_mode ) {
		mNightMode = night_mode;
		if ( mNightMode ) {
			IL::Camera::setFramerate( mConfig->integer( mConfigObject + ".night_fps", mConfig->integer( mConfigObject + ".fps", 60 ) ) );
			IL::Camera::setExposureValue( 0, 2.8f, ( mISO = mConfig->integer( mConfigObject + ".night_iso", 0 ) ), 0 );
			IL::Camera::setBrightness( mConfig->integer( mConfigObject + ".night_brightness", 80 ) );
			IL::Camera::setContrast( mConfig->integer( mConfigObject + ".night_contrast", 100 ) );
			IL::Camera::setSaturation( mConfig->integer( mConfigObject + ".night_saturation", -25 ) );
		} else {
			IL::Camera::setFramerate( mConfig->integer( mConfigObject + ".fps", 60 ) );
			IL::Camera::setExposureValue( 0, 2.8f, ( mISO = mConfig->integer( mConfigObject + ".iso", 0 ) ), 0 );
			IL::Camera::setBrightness( mConfig->integer( mConfigObject + ".brightness", 55 ) );
			IL::Camera::setContrast( mConfig->integer( mConfigObject + ".contrast", 0 ) );
			IL::Camera::setSaturation( mConfig->integer( mConfigObject + ".saturation", 8 ) );
		}
	}
}


std::string Raspicam::switchWhiteBalance()
{
	mWhiteBalanceLock = "";
	mWhiteBalance = (IL::Camera::WhiteBalControl)( ( mWhiteBalance + 1 ) % ( WhiteBalControlHorizon + 1 ) );
	IL::Camera::setWhiteBalanceControl( mWhiteBalance );
	return whiteBalance();
}


std::string Raspicam::lockWhiteBalance()
{
	OMX_CONFIG_CAMERASETTINGSTYPE settings;
	OMX_INIT_STRUCTURE( settings );
	settings.nPortIndex = 71;
	IL::Camera::GetConfig( OMX_IndexConfigCameraSettings, &settings );

	mWhiteBalance = WhiteBalControlOff;
	IL::Camera::setWhiteBalanceControl( mWhiteBalance );

	OMX_CONFIG_CUSTOMAWBGAINSTYPE awb;
	OMX_INIT_STRUCTURE( awb );
	awb.xGainR = settings.nRedGain << 8;
	awb.xGainB = settings.nBlueGain << 8;
	IL::Camera::SetConfig( OMX_IndexConfigCustomAwbGains, &awb );

	char ret[64];
	sprintf( ret, "locked (R:%.2f, B:%.2f)", ((float)awb.xGainR) / 65535.0f, ((float)awb.xGainB) / 65535.0f );
	mWhiteBalanceLock = std::string(ret);
	return mWhiteBalanceLock;
}


void Raspicam::setLensShader_internal( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B )
{
	uint8_t* grid = new uint8_t[52 * 39 * 4];
	for ( int32_t y = 0; y < 39; y++ ) {
		for ( int32_t x = 0; x < 52; x++ ) {
			int32_t r = R.base;
			int32_t g = G.base;
			int32_t b = B.base;
			int32_t dist = std::sqrt( ( x - 52 / 2 ) * ( x - 52 / 2 ) + ( y - 39 / 2 ) * ( y - 39 / 2 ) );
			if ( R.radius > 0 ) {
				float dot_r = (float)std::max( 0, R.radius - dist ) / (float)R.radius;
				r = std::max( 0, std::min( 255, r + (int32_t)( dot_r * R.strength ) ) );
			}
			if ( G.radius > 0 ) {
				float dot_g = (float)std::max( 0, G.radius - dist ) / (float)G.radius;
				g = std::max( 0, std::min( 255, g + (int32_t)( dot_g * G.strength ) ) );
			}
			if ( B.radius > 0 ) {
				float dot_b = (float)std::max( 0, B.radius - dist ) / (float)B.radius;
				b = std::max( 0, std::min( 255, b + (int32_t)( dot_b * B.strength ) ) );
			}
			grid[ (x + y * 52) + ( 52 * 39 ) * 0 ] = r;
			grid[ (x + y * 52) + ( 52 * 39 ) * 1 ] = g;
			grid[ (x + y * 52) + ( 52 * 39 ) * 2 ] = g;
			grid[ (x + y * 52) + ( 52 * 39 ) * 3 ] = b;
		}
	}
	IL::Camera::setLensShadingGrid( 64, 52, 39, grid );
}


void Raspicam::setLensShader( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B )
{
	mLensShaderR = R;
	mLensShaderG = G;
	mLensShaderB = B;

	IL::Camera::SetCapturing( false );
	IL::Camera::SetState( Component::StateIdle );
	IL::Camera::SetState( Component::StateLoaded );
	setLensShader_internal( R, G, B );
	IL::Camera::SetState( Component::StateIdle );
	IL::Camera::SetState( Component::StateExecuting );
	IL::Camera::SetCapturing( true );
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
	}
}


void Raspicam::TakePicture()
{
	if ( mTakingPicture ) {
		return;
	}
	std::unique_lock<std::mutex> locker( mTakePictureMutex );
	mTakingPicture = true;
	mTakePictureCond.notify_all();
}


bool Raspicam::TakePictureThreadRun()
{
	std::unique_lock<std::mutex> locker( mTakePictureMutex );
	std::cv_status timeout = mTakePictureCond.wait_for( locker, std::chrono::milliseconds( 1 * 1000 ) );
	locker.unlock();
	if ( timeout == std::cv_status::timeout ) {
		return true;
	}

	uint8_t* buf = new uint8_t[8*1024*1024];
	uint32_t buflen = 0;
	bool end_of_frame = false;

	IL::Camera::SetCapturing( true, 72 );
	do {
		buflen += mImageEncoder->getOutputData( &buf[buflen], &end_of_frame, true );
	} while ( end_of_frame == false );
	IL::Camera::SetCapturing( true );

	char filename[256];
	uint32_t fileid = 0;
	DIR* dir;
	struct dirent* ent;
	if ( ( dir = opendir( "/var/PHOTO" ) ) != nullptr ) {
		while ( ( ent = readdir( dir ) ) != nullptr ) {
			std::string file = std::string( ent->d_name );
			uint32_t id = std::atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
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

	gDebug() << "Picture taken !\n";
	delete[] buf;
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
			gDebug() << "Raspicam connected !\n";
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
			LiveSend( (char*)mLiveBuffer, datalen );
		}
		if ( mRecording and not mBetterRecording ) {
			RecordWrite( (char*)mLiveBuffer, datalen );
		}
	}

	// Send video headers every 10 seconds
	if ( Board::GetTicks() - mHeadersTick >= 1000 * 1000 * 10 ) {
		mHeadersTick = Board::GetTicks();
		const std::map< uint32_t, uint8_t* > headers = mEncoder->headers();
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

	if ( mRecordStream ) {
		mRecordStreamMutex.lock();
		while ( mRecordStreamQueue.size() > 0 ) {
			std::pair< uint8_t*, uint32_t > frame = mRecordStreamQueue.front();
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
		mRecordStreamMutex.unlock();
	}

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
		gDebug() << "Link->Write() error : " << strerror(errno) << " (" << errno << ")\n";
		return -1;
	}
	return 0;
}


int Raspicam::RecordWrite( char* data, int datalen, int64_t pts, bool audio )
{
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
				std::string file = std::string( ent->d_name );
				uint32_t id = std::atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
				if ( id >= fileid ) {
					fileid = id + 1;
				}
			}
			closedir( dir );
		}
		sprintf( filename, "/var/VIDEO/video_%02dfps_%06u.h264", IL::Camera::framerate(), fileid );
		mRecordFilename = std::string( filename );
		FILE* stream = fopen( filename, "wb" );
		const std::map< uint32_t, uint8_t* > headers = mEncoder->headers();
		if ( headers.size() > 0 ) {
			for ( auto hdr : headers ) {
				fwrite( hdr.second, 1, hdr.first, stream );
			}
		}
		mRecordStream = stream;
	}

	std::pair< uint8_t*, uint32_t > frame;
	frame.first = new uint8_t[datalen];
	frame.second = datalen;
	memcpy( frame.first, data, datalen );
	mRecordStreamMutex.lock();
	mRecordStreamQueue.emplace_back( frame );
	mRecordStreamMutex.unlock();

	return ret;
}


uint32_t* Raspicam::getFileSnapshot( const std::string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp )
{
	uint32_t* data = nullptr;
	*width = 0;
	*height = 0;
	*bpp = 0;

	std::ifstream file( filename, std::ios_base::in | std::ios_base::binary );
	if ( file.is_open() ) {
		IL::VideoDecode* decoder = new IL::VideoDecode( 60, IL::VideoDecode::CodingAVC, true );
		decoder->setRGB565Mode( true );
		uint32_t buf[2048];
		uint32_t frame = 0;
		decoder->SetState( IL::Component::StateExecuting );
		while ( not file.eof() and ( not decoder->valid() or frame < 32 ) ) {
			std::streamsize sz = file.readsome( (char*)buf, sizeof(buf) );
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
		decoder->SetState( IL::Component::StateIdle );
		delete decoder;
	}

	return data;
}

#endif // BOARD_rpi
