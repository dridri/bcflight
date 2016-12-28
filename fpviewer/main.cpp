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
#include "Stream.h"
#include "GLContext.h"
#include "RendererHUD.h"
#include "RendererHUDNeo.h"

int main( int ac, char** av )
{
	Stream* stream = nullptr;
	Link* stream_link = nullptr;
	Controller* controller = nullptr;
	Link* controller_link = nullptr;

	if ( ac <= 1 ) {
		std::cout << "FATAL ERROR : No config file specified !\n";
		return -1;
	}

#ifdef BOARD_rpi
	bcm_host_init();
#endif

	Config* config = new Config( av[1] );
	config->Reload();

	if ( config->string( "stream.link.link_type" ) == "Socket" ) {
		stream_link = new ::Socket( config->string( "stream.link.address", "192.168.32.1" ), config->integer( "stream.link.port", 2020 ) );
	} else { // default to RawWifi
		stream_link = new RawWifi( config->string( "stream.link.device", "wlan0" ), config->integer( "stream.link.output_port", 10 ), config->integer( "stream.link.input_port", 11 ), config->integer( "stream.link.read_timeout", 1 ) );
		dynamic_cast< RawWifi* >( stream_link )->setBlocking( config->boolean( "stream.link.blocking", true ) );
		dynamic_cast< RawWifi* >( stream_link )->setCECMode( config->string( "stream.link.cec_mode", "none" ) );
		dynamic_cast< RawWifi* >( stream_link )->setBlockRecoverMode( config->string( "stream.link.block_recover", "contiguous" ) );
	}

	if ( config->string( "controller.link.link_type" ) == "Socket" ) {
		controller_link = new ::Socket( config->string( "controller.link.address", "192.168.32.1" ), config->integer( "controller.link.port", 2020 ) );
	} else if ( config->string( "controller.link.link_type" ) == "RawWifi" ) {
		controller_link = new RawWifi( config->string( "controller.link.device", "wlan0" ), -1, config->integer( "controller.link.input_port", 1 ), -1 );
	}
	if ( controller_link ) {
		// Create a controller in spectate mode
		controller = new Controller( controller_link, true );
	}

	stream = new Stream( stream_link, config->integer( "stream.width", 1920 ), config->integer( "stream.height", 1080 ), config->boolean( "stream.stereo", true ) );
	stream->Run();

	GLContext* glContext = new GLContext();
	glContext->Initialize( 1280, 720 );
	RendererHUD* mRendererHUD = new RendererHUDNeo( 1280, 720 );

	while ( 1 ) {
		glClear( GL_COLOR_BUFFER_BIT );

		VideoStats video_stats = {
			.fps = stream->fps(),
		};
		IwStats iwstats = {
			.qual = stream_link->RxQuality(),
			.level = stream_link->RxLevel(),
			.noise = 0,
			.channel = stream_link->Channel(),
			.source = 0,
		};
		mRendererHUD->PreRender( &video_stats );
		mRendererHUD->Render( controller, &video_stats, &iwstats );

		uint64_t t = Thread::GetTick();
		uint32_t minuts = t / ( 1000 * 60 );
		uint32_t seconds = ( t / 1000 ) % 60;
		uint32_t ms = t % 1000;
		mRendererHUD->RenderText( 200, 400, std::to_string(minuts) + ":" + std::to_string(seconds) + ":" + std::to_string(ms), 0xFFFFFFFF );

		glContext->SwapBuffers();
	}

	std::cout << "Exiting (this should not happen)\n";
	return 0;
}
