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

#include <execinfo.h>
#include <signal.h>

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <iostream>
#include <Link.h>
#include <RawWifi.h>
#include <Socket.h>
#include "Config.h"
#include "Board.h"
#include "Stream.h"
#include "AudioOutput.h"
#include "GLContext.h"
#include "RendererHUD.h"
#include "RendererHUDNeo.h"

int main( int ac, char** av )
{
	pthread_setname_np( pthread_self(), "main" );

	Stream* stream = nullptr;
	Link* stream_link = nullptr;
	Link* audio_link = nullptr;
	Controller* controller = nullptr;
	Link* controller_link = nullptr;

	if ( ac <= 1 ) {
		std::cout << "FATAL ERROR : No config file specified !\n";
		return -1;
	}

	Board* board = new Board();

#ifdef BOARD_rpi
	bcm_host_init();
#endif

	Config* config = new Config( av[1] );
	config->Reload();

	if ( config->string( "stream.link.link_type" ) == "Socket" ) {
		stream_link = new ::Socket( config->string( "stream.link.address", "192.168.32.1" ), config->integer( "stream.link.port", 2021 ) );
		audio_link = new ::Socket( config->string( "audio.link.address", "192.168.32.1" ), config->integer( "audio.link.port", 2022 ) );
	} else { // default to RawWifi
		stream_link = new RawWifi( config->string( "stream.link.device", "wlan0" ), config->integer( "stream.link.output_port", 10 ), config->integer( "stream.link.input_port", 11 ), config->integer( "stream.link.read_timeout", 1 ) );
		dynamic_cast< RawWifi* >( stream_link )->SetChannel( config->integer( "stream.link.channel", 11 ) );
		dynamic_cast< RawWifi* >( stream_link )->setBlocking( config->boolean( "stream.link.blocking", true ) );
		dynamic_cast< RawWifi* >( stream_link )->setCECMode( config->string( "stream.link.cec_mode", "none" ) );
		dynamic_cast< RawWifi* >( stream_link )->setBlockRecoverMode( config->string( "stream.link.block_recover", "contiguous" ) );
		audio_link = new RawWifi( config->string( "audio.link.device", "wlan0" ), config->integer( "audio.link.output_port", 20 ), config->integer( "audio.link.input_port", 21 ), config->integer( "audio.link.read_timeout", 1 ) );
		dynamic_cast< RawWifi* >( audio_link )->SetChannel( config->integer( "audio.link.channel", 11 ) );
		dynamic_cast< RawWifi* >( audio_link )->setBlocking( config->boolean( "audio.link.blocking", true ) );
		dynamic_cast< RawWifi* >( audio_link )->setCECMode( config->string( "audio.link.cec_mode", "none" ) );
		dynamic_cast< RawWifi* >( audio_link )->setBlockRecoverMode( config->string( "audio.link.block_recover", "none" ) );
	}

	if ( config->string( "controller.link.link_type" ) == "Socket" ) {
		controller_link = new ::Socket( config->string( "controller.link.address", "192.168.32.1" ), config->integer( "controller.link.port", 2020 ) );
	} else if ( config->string( "controller.link.link_type" ) == "RawWifi" ) {
		controller_link = new RawWifi( config->string( "controller.link.device", "wlan0" ), -1, config->integer( "controller.link.input_port", 1 ), -1 );
		dynamic_cast< RawWifi* >( controller_link )->SetChannel( config->integer( "controller.link.channel", 11 ) );
	}
	if ( controller_link ) {
		// Create a controller in spectate mode
		controller = new Controller( controller_link, true );
	}

	bool stereo = config->boolean( "stream.stereo", true );
	bool direct_render = config->boolean( "stream.direct_render", true );
	stream = new Stream( stream_link, board->displayWidth(), board->displayHeight(), config->number( "stream.zoom", 1.0f ), config->boolean( "stream.stretch", false ), stereo, direct_render );
	stream->Start();

	AudioOutput* audio = new AudioOutput( audio_link, config->integer( "audio.channels", 1 ), config->integer( "audio.sample_rate", 44100 ), config->string( "audio.device", "default" ) );
	audio->Start();

	GLContext* glContext = new GLContext();
	glContext->Initialize( 1280, 720 );
	RendererHUD* mRendererHUD = new RendererHUDNeo( board->displayWidth(), board->displayHeight(), config->number( "hud.ratio", 1.0f ), config->integer( "hud.font_size", 28 ), config->boolean( "hud.correction", true ) );
	mRendererHUD->setStereo( stereo );
	mRendererHUD->set3DStrength( config->number( "hud.stereo_strength", 0.004f ) );

	uint64_t time_base = Thread::GetTick();
	uint64_t time = 0;
	bool last_armed = false;

	while ( 1 ) {
		glClear( GL_COLOR_BUFFER_BIT );
		stream->Render( mRendererHUD );

		VideoStats video_stats = {
			.width = (int)stream->width(),
			.height = (int)stream->height(),
			.fps = stream->fps(),
		};
		IwStats iwstats = {
			.qual = stream_link->RxQuality(),
			.level = stream_link->RxLevel(),
			.noise = 0,
			.channel = stream_link->Channel(),
			.source = 0,
		};
		if ( controller ) {
			mRendererHUD->setNightMode( controller->nightMode() );
		}
		mRendererHUD->PreRender( &video_stats );
		mRendererHUD->Render( controller, board->localBatteryVoltage(), &video_stats, &iwstats );
/*
		if ( controller and controller->armed() ) {
			if ( last_armed == false ) {
				time_base = Thread::GetTick();
				last_armed = true;
			}
			time = Thread::GetTick() - time_base;
		} else {
			last_armed = false;
		}

		uint32_t minuts = time / ( 1000 * 60 );
		uint32_t seconds = ( time / 1000 ) % 60;
		uint32_t ms = time % 1000;
		char txt[256];
		sprintf( txt, "%02d:%02d:%03d", minuts, seconds, ms );
		mRendererHUD->RenderText( 1280 * 0.365f, 720 - 240, txt, 0xFFFFFFFF, 1.0f, true );
*/
		glContext->SwapBuffers();
	}

	std::cout << "Exiting (this should not happen)\n";
	return 0;
}
