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

#if ( defined( BOARD_rpi ) )

#include <dirent.h>
#include "AlsaMic.h"
#include <Main.h>
#include <Debug.h>
#include <Link.h>
#include <Recorder.h>
#include <video/Camera.h>

int AlsaMic::flight_register( Main* main )
{
	RegisterMicrophone( "AlsaMic", &AlsaMic::Instanciate );
	return 0;
}


Microphone* AlsaMic::Instanciate( Config* config, const std::string& object )
{
	return new AlsaMic( config, object );
}


AlsaMic::AlsaMic( Config* config, const std::string& conf_obj )
	: Microphone()
	, mRecordSyncCounter( 0 )
	, mRecordStream( nullptr )
	, mRecorderTrackId( 0 )
{
	int err = 0;
	std::string device = config->string( conf_obj + ".device", "plughw:1,0" );
	unsigned int rate = config->integer( conf_obj + ".sample_rate", 44100 );
	unsigned int channels = config->integer( conf_obj + ".channels", 1 );
	snd_pcm_hw_params_t* hw_params;

	if ( ( err = snd_pcm_open( &mPCM, device.c_str(), SND_PCM_STREAM_CAPTURE, 0 ) ) < 0 ) {
		fprintf( stderr, "cannot open audio device %s (%s)\n", device.c_str(), snd_strerror( err ) );
		Board::defectivePeripherals()["Microphone"] = true;
		return;
	}
	fprintf( stdout, "audio interface opened\n");

	if ( ( err = snd_pcm_hw_params_malloc( &hw_params ) ) < 0 ) {
		fprintf( stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params allocated\n");

	if ( ( err = snd_pcm_hw_params_any( mPCM, hw_params ) ) < 0 ) {
		fprintf( stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params initialized\n");

	if ( ( err = snd_pcm_hw_params_set_access( mPCM, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 ) {
		fprintf( stderr, "cannot set access type (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params access setted\n");

	if ( ( err = snd_pcm_hw_params_set_format( mPCM, hw_params, SND_PCM_FORMAT_S16_LE ) ) < 0 ) {
		fprintf( stderr, "cannot set sample format (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params format setted\n");

	if ( ( err = snd_pcm_hw_params_set_rate_near( mPCM, hw_params, &rate, 0 ) ) < 0 ) {
		fprintf( stderr, "cannot set sample rate (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params rate setted to %d\n", rate);

	if ( ( err = snd_pcm_hw_params_set_channels( mPCM, hw_params, channels ) ) < 0 ) {
		fprintf( stderr, "cannot set channel count (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params channels setted to 1\n");
/*
	snd_pcm_uframes_t framesize = 1024;
	if ( ( err = snd_pcm_hw_params_set_period_size_near( mPCM, hw_params, &framesize, 0 ) ) < 0 ) {
		fprintf( stderr, "cannot set frame size (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params frame size setted to %u\n", framesize);

	if ( ( err = snd_pcm_hw_params_set_periods( mPCM, hw_params, 1, 1 ) ) < 0 ) {
		fprintf( stderr, "cannot set periods (%s)\n", snd_strerror( err ) );
// 		return;
	}
	fprintf( stdout, "hw_params periods setted to 1\n");

	snd_pcm_uframes_t bufsize = 1024;
	if ( ( err = snd_pcm_hw_params_set_buffer_size_near( mPCM, hw_params, &bufsize ) ) < 0 ) {
		fprintf( stderr, "cannot set buffer size (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params buffer size setted to %u\n", bufsize);
*/
	if ( ( err = snd_pcm_hw_params( mPCM, hw_params ) ) < 0 ) {
		fprintf( stderr, "cannot set parameters (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params setted\n" );

	snd_pcm_hw_params_free( hw_params );
	fprintf( stdout, "hw_params freed\n" );
		
	if ( ( err = snd_pcm_prepare ( mPCM ) ) < 0 ) {
		fprintf( stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror( err ) );
		return;
	}

	fprintf( stdout, "Audio interface prepared\n" );

	shine_set_config_mpeg_defaults( &mShineConfig.mpeg );
	mShineConfig.wave.channels = ( ( channels == 2 ) ? PCM_STEREO : PCM_MONO );
	mShineConfig.wave.samplerate = rate;
	mShineConfig.mpeg.mode = ( ( channels == 2 ) ? STEREO : MONO );
	mShineConfig.mpeg.bitr = 128;
	mShine = shine_initialise( &mShineConfig );
	printf ("shine_samples_per_pass : %d\n", shine_samples_per_pass( mShine ) );
	if ( Main::instance()->recorder() ) {
		mRecorderTrackId = Main::instance()->recorder()->AddAudioTrack( 1, rate, "mp3" );
	}

	mLink = Link::Create( config, conf_obj + ".link" );
	mLiveThread = new HookThread<AlsaMic>( "microphone", this, &AlsaMic::LiveThreadRun );
	mLiveThread->Start();
	mLiveThread->setPriority( 90, 3 );
}


AlsaMic::~AlsaMic()
{
}


bool AlsaMic::LiveThreadRun()
{
	if ( mLink and not mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			gDebug() << "Microphone connected !\n";
			mLink->setBlocking( false );
		}
		return true;
	}

	uint8_t data[32768];
	snd_pcm_sframes_t size = snd_pcm_readi( mPCM, data, 1152 ) * 2;

	if ( size > 0 ) {
		if ( mLink ) {
			mLink->Write( data, size, false, 0 );
		}
		if ( Main::instance()->camera() and Main::instance()->camera()->recording() ) {
			RecordWrite( (char*)data, size );
		}
	} else {
		printf( "snd_pcm_readi error %ld\n", size / 2 );
		if ( size / 2 == -EPIPE ) {
			snd_pcm_recover( mPCM, (int)size / 2, 0 );
		} else {
			Board::defectivePeripherals()["Microphone"] = true;
			return false;
		}
	}
	return true;
}


int AlsaMic::RecordWrite( char* data, int datalen )
{
	int baselen = datalen;
	datalen = 0;
	data = (char*)shine_encode_buffer_interleaved( mShine, (int16_t*)data, &datalen );

	Recorder* recorder = Main::instance()->recorder();
	if ( recorder ) {
		recorder->WriteSample( mRecorderTrackId, Board::GetTicks(), data, datalen );
		return datalen;
	}

	int ret = 0;

	if ( !mRecordStream ) {
		char filename[256];
		std::string file = Main::instance()->camera()->recordFilename();
		if ( file == "" ) {
			return 0;
		}
		uint32_t fileid = std::atoi( file.substr( file.rfind( "_" ) + 1 ).c_str() );
		sprintf( filename, "/var/VIDEO/audio_44100hz_1ch_%06u.mp3", fileid );
		mRecordStream = fopen( filename, "wb" );
	}

	ret = fwrite( data, 1, datalen, mRecordStream );

	mRecordSyncCounter = ( mRecordSyncCounter + 1 ) % 2048;
	if ( mRecordSyncCounter % 15 == 0 ) { // sync on disk every 15 frames (up to 15*512*1/44100 seconds)
		fflush( mRecordStream );
		fsync( fileno( mRecordStream ) );
	}

	return ret;
}

#endif // BOARD_rpi
