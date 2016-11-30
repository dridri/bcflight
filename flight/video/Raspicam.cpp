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

Raspicam::Raspicam( Config* config, const std::string& conf_obj )
	: ::Camera()
	, IL::Camera( config->integer( conf_obj + ".width", 1280 ), config->integer( conf_obj + ".height", 720 ), 0, true, true )
	, mConfig( config )
	, mConfigObject( conf_obj )
	, mLiveFrameCounter( 0 )
	, mLedTick( 0 )
	, mLedState( true )
	, mRecording( false )
	, mRecordStream( nullptr )
{
	fDebug( config, conf_obj );

	IL::Camera::setMirror( true, false );
// 	IL::Camera::setWhiteBalanceControl( IL::Camera::WhiteBalControlAuto );
	IL::Camera::setWhiteBalanceControl( IL::Camera::WhiteBalControlHorizon );
	IL::Camera::setExposureControl( IL::Camera::ExposureControlLargeAperture );
	IL::Camera::setExposureValue( mConfig->integer( mConfigObject + ".exposure", 0 ), mConfig->integer( mConfigObject + ".aperture", 2.8f ), mConfig->integer( mConfigObject + ".iso", 0 ), mConfig->integer( mConfigObject + ".shutter_speed", 0 ) );
	IL::Camera::setSharpness( mConfig->integer( mConfigObject + ".sharpness", 100 ) );
	IL::Camera::setFramerate( mConfig->integer( mConfigObject + ".fps", 60 ) );
	IL::Camera::setBrightness( mConfig->integer( mConfigObject + ".brightness", 55 ) );
	IL::Camera::setSaturation( mConfig->integer( mConfigObject + ".saturation", 8 ) );
	IL::Camera::setContrast( mConfig->integer( mConfigObject + ".contrast", 0 ) );
	IL::Camera::setFrameStabilisation( mConfig->boolean( mConfigObject + ".stabilisation", false ) );

	mEncoder = new IL::VideoEncode( mConfig->integer( mConfigObject + ".kbps", 1024 ), IL::VideoEncode::CodingAVC, true );
	IL::Camera::SetupTunnel( 71, mEncoder, 200 );

	mLiveTicks = 0;
	mRecordTicks = 0;
	mRecordFrameData = nullptr;
	mRecordFrameDataSize = 0;
	mRecordFrameSize = 0;

	mLink = Link::Create( config, conf_obj + ".link" );

	mLiveThread = new HookThread<Raspicam>( "cam_live", this, &Raspicam::LiveThreadRun );
	mLiveThread->Start();
	mLiveThread->setPriority( 99, 1 );
}


Raspicam::~Raspicam()
{
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
			delete mRecordStream;
			mRecordStream = nullptr;
		}
	}
}


bool Raspicam::LiveThreadRun()
{
	if ( !mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Raspicam connected !\n";
			mLink->setBlocking( false );
			IL::Camera::SetState( IL::Component::StateExecuting );
			mEncoder->SetState( IL::Component::StateExecuting );
			mLiveFrameCounter = 0;
		}
		return true;
	} else if ( mRecording and Board::GetTicks() - mLedTick >= 1000 * 500 ) {
		GPIO::Write( 32, ( mLedState = !mLedState ) );
		mLedTick = Board::GetTicks();
	}

	uint8_t data[65536] = { 0 };
	uint32_t datalen = mEncoder->getOutputData( data );
	if ( (int32_t)datalen > 0 ) {
		mLiveFrameCounter++;
		LiveSend( (char*)data, datalen );
		if ( mRecording ) {
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

	int err = mLink->Write( (uint8_t*)data, datalen, 0 );

	if ( err < 0 ) {
		gDebug() << "Link->Write() error : " << strerror(errno) << " (" << errno << ")\n";
		return -1;
	}
	return 0;
}


int Raspicam::RecordWrite( char* data, int datalen, int64_t pts, bool audio )
{
	int ret = 0;

	if ( !mRecordStream ) {
		char filename[256];
		uint32_t fileid = 0;
		DIR* dir;
		struct dirent* ent;
		if ( ( dir = opendir( "/var/VIDEO" ) ) != nullptr ) {
			while ( ( ent = readdir( dir ) ) != nullptr ) {
				std::string file = std::string( ent->d_name );
				uint32_t id = std::atoi( file.substr( file.find( "_" ) + 1 ).c_str() );
				if ( id >= fileid ) {
					fileid = id + 1;
				}
			}
			closedir( dir );
		}
		sprintf( filename, "/var/VIDEO/video_%06u.h264", fileid );
		mRecordStream = new std::ofstream( filename );
	}

	auto pos = mRecordStream->tellp();
	mRecordStream->write( data, datalen );
	ret = mRecordStream->tellp() - pos;
	mRecordStream->flush();

	return ret;
}

#endif // BOARD_rpi
