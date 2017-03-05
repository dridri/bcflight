/*
 * BCFlight
 * Copyright (C) 2017 Adrien Aubry (drich)
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

#include "AudioOutput.h"

AudioOutput::AudioOutput( Link* link, uint8_t channels, uint32_t samplerate, std::string device )
	: Thread( "AudioOutput" )
	, mLink( link )
	, mPCM( nullptr )
	, mChannels( channels )
{
	int err = 0;
	snd_pcm_hw_params_t* hw_params;

	if ( ( err = snd_pcm_open( &mPCM, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0 | SND_PCM_NONBLOCK ) ) < 0 ) {
		fprintf( stderr, "cannot open audio device %s (%s)\n", device.c_str(), snd_strerror( err ) );
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

	if ( ( err = snd_pcm_hw_params_set_rate_near( mPCM, hw_params, &samplerate, 0 ) ) < 0 ) {
		fprintf( stderr, "cannot set sample rate (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params rate setted to %d\n", samplerate);

	if ( ( err = snd_pcm_hw_params_set_channels( mPCM, hw_params, channels ) ) < 0 ) {
		fprintf( stderr, "cannot set channel count (%s)\n", snd_strerror( err ) );
		return;
	}
	fprintf( stdout, "hw_params channels setted to 1\n");

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
}


AudioOutput::~AudioOutput()
{
}


bool AudioOutput::run()
{
	if ( not mLink->isConnected() ) {
		mLink->Connect();
		if ( mLink->isConnected() ) {
			uint32_t uid = htonl( 0x12345678 );
			mLink->Write( &uid, sizeof(uid), 0 );
		} else {
			usleep( 1000 * 1000 );
		}
		return true;
	}

	const uint32_t bufsize = 32768;
	uint8_t buffer[bufsize] = { 0 };

	int32_t size = mLink->Read( buffer, bufsize, 0 );

	if ( size > 0 ) {
		int ret = snd_pcm_writei( mPCM, buffer, size / 2 / mChannels );
		if ( ret == -EPIPE ) {
			snd_pcm_prepare( mPCM );
		}
	}

	return true;
}
