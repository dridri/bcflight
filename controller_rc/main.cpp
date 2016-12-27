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

#include <iostream>
#include <Link.h>
#include <RawWifi.h>
#include <Socket.h>
#include "Stream.h"
#include "Config.h"
#include "ControllerClient.h"
#include "ui/GlobalUI.h"

int main( int ac, char** av )
{
	Controller* controller = nullptr;
	Stream* stream = nullptr;
	Link* controller_link = nullptr;
	Link* stream_link = nullptr;

	if ( ac <= 1 ) {
		std::cout << "FATAL ERROR : No config file specified !\n";
		return -1;
	}
/*
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 0, &mask );
	sched_setaffinity( getpid(), sizeof(mask), &mask );
*/
#ifdef BOARD_rpi
	bcm_host_init();
#endif

	Config* config = new Config( av[1] );
	config->Reload();

	if ( config->string( "controller.link.link_type" ) == "Socket" ) {
		controller_link = new ::Socket( config->string( "controller.link.address", "192.168.32.1" ), config->integer( "controller.link.port", 2020 ) );
	} else if ( config->string( "controller.link.link_type" ) == "RawWifi" ) {
		controller_link = new RawWifi( config->string( "controller.link.device", "wlan0" ), config->integer( "controller.link.output_port", 0 ), config->integer( "controller.link.input_port", 1 ), -1 );
	}

	if ( config->string( "stream.link.link_type" ) == "Socket" ) {
		stream_link = new ::Socket( config->string( "stream.link.address", "192.168.32.1" ), config->integer( "stream.link.port", 2020 ) );
	} else if ( config->string( "stream.link.link_type" ) == "RawWifi" ) {
		stream_link = new RawWifi( config->string( "stream.link.device", "wlan0" ), config->integer( "stream.link.output_port", 10 ), config->integer( "stream.link.input_port", 11 ), config->integer( "stream.link.read_timeout", 1 ) );
		dynamic_cast< RawWifi* >( stream_link )->setBlocking( config->boolean( "stream.link.blocking", true ) );
		dynamic_cast< RawWifi* >( stream_link )->setCECMode( config->string( "stream.link.cec_mode", "none" ) );
		dynamic_cast< RawWifi* >( stream_link )->setBlockRecoverMode( config->string( "stream.link.block_recover", "contiguous" ) );
	}

	if ( controller_link ) {
// 		controller = new ControllerClient( controller_link, config->boolean( "controller.spectate", false ) );
	}
	if ( stream_link ) {
		stream = new Stream( controller, stream_link, config->integer( "stream.width", 1920 ), config->integer( "stream.height", 1080 ), config->boolean( "stream.stereo", true ) );
		stream->setStereo( config->boolean( "stream.stereo", true ) );
		stream->setRenderHUD( config->boolean( "stream.hud", true ) );
	}

// 	GlobalUI* ui = new GlobalUI();
// 	ui->Start();

	if ( stream ) { 
		stream->Run();
	}

	while ( 1 ) {
		usleep( 1000 * 1000 );
	}
	std::cout << "Exiting (this should not happen)\n";
	return 0;
}
