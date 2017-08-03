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


Raspicam::Raspicam( Config* config, const std::string& conf_obj )
	: ::Camera()
	, IL::Camera( config->integer( conf_obj + ".width", 1280 ), config->integer( conf_obj + ".height", 720 ), 0, true, config->integer( conf_obj + ".sensor_mode", 0 ), true )
	, mConfig( config )
	, mConfigObject( conf_obj )
	, mLink( nullptr )
	, mDirectMode( config->boolean( conf_obj + ".direct_mode", false ) )
	, mLiveFrameCounter( 0 )
	, mLedTick( 0 )
	, mHeadersTick( 0 )
	, mLedState( true )
	, mNightMode( false )
	, mPaused( false )
	, mWhiteBalance( WhiteBalControlAuto )
	, mRecording( false )
	, mSplitter( nullptr )
	, mEncoderRecord( nullptr )
	, mRecordThread( nullptr )
	, mTakingPicture( false )
	, mRecordStream( nullptr )
{
	fDebug( config, conf_obj );

	if ( not IL::Component::mHandle ) {
		Board::defectivePeripherals()["Camera"] = true;
		return;
	}

	// TEST : record everytime
	// TODO : remove this
	mRecording = true;

	mBetterRecording = ( config->integer( conf_obj + ".record_kbps", config->integer( conf_obj + ".kbps", 1024 ) ) != config->integer( conf_obj + ".kbps", 1024 ) );
	gDebug() << "Camera splitted encoders : " << mBetterRecording << "\n";

	if ( mConfig->boolean( mConfigObject + ".disable_lens_shading", false ) ) {
		IL::Camera::disableLensShading();
	}
/*
	if ( mConfig->integer( mConfigObject + ".lens_shading_grid.width", 0 ) > 0 ) {
		int width = mConfig->integer( mConfigObject + ".lens_shading_grid.width", 0 );
		int height = mConfig->integer( mConfigObject + ".lens_shading_grid.height", 0 );
		int cell_size = mConfig->integer( mConfigObject + ".lens_shading_grid.cell_size", 0 );
		mConfig->integerArray( mConfigObject + ".lens_shading_grid.grid" ); // TODO
	}
*/
	IL::Camera::setMirror( mConfig->boolean( mConfigObject + ".hflip", false ), mConfig->boolean( mConfigObject + ".vflip", false ) );
	IL::Camera::setWhiteBalanceControl( IL::Camera::WhiteBalControlAuto );
// 	IL::Camera::setWhiteBalanceControl( IL::Camera::WhiteBalControlHorizon );
	IL::Camera::setExposureControl( IL::Camera::ExposureControlAuto );
// 	IL::Camera::setExposureControl( IL::Camera::ExposureControlLargeAperture );
	IL::Camera::setExposureValue( mConfig->integer( mConfigObject + ".exposure", 0 ), mConfig->integer( mConfigObject + ".aperture", 2.8f ), mConfig->integer( mConfigObject + ".iso", 0 ), mConfig->integer( mConfigObject + ".shutter_speed", 0 ) );
	IL::Camera::setSharpness( mConfig->integer( mConfigObject + ".sharpness", 100 ) );
	IL::Camera::setFramerate( mConfig->integer( mConfigObject + ".fps", 60 ) );
	IL::Camera::setBrightness( mConfig->integer( mConfigObject + ".brightness", 55 ) );
	IL::Camera::setSaturation( mConfig->integer( mConfigObject + ".saturation", 8 ) );
	IL::Camera::setContrast( mConfig->integer( mConfigObject + ".contrast", 0 ) );
	IL::Camera::setFrameStabilisation( mConfig->boolean( mConfigObject + ".stabilisation", false ) );

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

	mLink = Link::Create( config, conf_obj + ".link" );

	mLiveThread = new HookThread<Raspicam>( "cam_live", this, &Raspicam::LiveThreadRun );
	mLiveThread->Start();
	if ( not mDirectMode ) {
		mLiveThread->setPriority( 99, 3 );
	}

	mRecordThread = new HookThread<Raspicam>( "cam_record", this, &Raspicam::RecordThreadRun );
	mRecordThread->Start();

	mTakePictureThread->Start();
}


Raspicam::~Raspicam()
{
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


const bool Raspicam::nightMode()
{
	return mNightMode;
}


const std::string Raspicam::whiteBalance()
{
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


void Raspicam::setNightMode( bool night_mode )
{
	if ( mNightMode != night_mode ) {
		mNightMode = night_mode;
		if ( mNightMode ) {
			IL::Camera::setFramerate( mConfig->integer( mConfigObject + ".night_fps", mConfig->integer( mConfigObject + ".fps", 60 ) ) );
	// 		IL::Camera::setExposureControl( IL::Camera::ExposureControlNight );
// 			IL::Camera::setWhiteBalanceControl( IL::Camera::WhiteBalControlAuto );
			IL::Camera::setBrightness( mConfig->integer( mConfigObject + ".night_brightness", 80 ) );
			IL::Camera::setContrast( mConfig->integer( mConfigObject + ".night_contrast", 100 ) );
			IL::Camera::setSaturation( mConfig->integer( mConfigObject + ".night_saturation", -25 ) );
		} else {
			IL::Camera::setFramerate( mConfig->integer( mConfigObject + ".fps", 60 ) );
// 			IL::Camera::setExposureControl( IL::Camera::ExposureControlAuto );
// 			IL::Camera::setWhiteBalanceControl( IL::Camera::WhiteBalControlHorizon );
			IL::Camera::setBrightness( mConfig->integer( mConfigObject + ".brightness", 55 ) );
			IL::Camera::setContrast( mConfig->integer( mConfigObject + ".contrast", 0 ) );
			IL::Camera::setSaturation( mConfig->integer( mConfigObject + ".saturation", 8 ) );
		}
	}
}


std::string Raspicam::switchWhiteBalance()
{
	mWhiteBalance = (IL::Camera::WhiteBalControl)( ( mWhiteBalance + 1 ) % ( WhiteBalControlHorizon + 1 ) );
	IL::Camera::setWhiteBalanceControl( mWhiteBalance );
	return whiteBalance();
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

	uint8_t* buf = new uint8_t[4*1024*1024];
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

	uint8_t data[65536] = { 0 };
	uint8_t* pData = data;
	/*
	mEncoder->dataAvailable( true );
	mEncoder->fillBuffer();
	uint32_t datalen = mEncoder->bufferLength();
	pData = mEncoder->buffer();
	*/
	uint32_t datalen = mEncoder->getOutputData( pData, true );

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
			LiveSend( (char*)pData, datalen );
		}
		if ( mRecording and not mBetterRecording ) {
			RecordWrite( (char*)pData, datalen );
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
			fwrite( frame.first, 1, frame.second, mRecordStream );
			delete frame.first;
			if ( mRecordSyncCounter == 0 ) {
				fflush( mRecordStream );
				fsync( fileno( mRecordStream ) );
			}
			mRecordSyncCounter = ( mRecordSyncCounter + 1 ) % 10; // sync on disk every 10 frames (up to 10*1/FPS seconds)
			mRecordStreamMutex.lock();
		}
		mRecordStreamMutex.unlock();
	}

	if ( mEncoderRecord ) {
		uint8_t data[65536] = { 0 };
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
// 	Recorder* recorder = Main::instance()->recorder();
// 	if ( recorder ) {
// 		recorder->WriteVideo( data, datalen );
// 		return datalen;
// 	}

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
