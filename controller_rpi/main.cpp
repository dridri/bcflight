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

#include <gammaengine/Instance.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include <gammaengine/Window.h>
#include <gammaengine/FramebufferWindow.h>
#include <gammaengine/Input.h>
#include <gammaengine/Renderer2D.h>
#include <gammaengine/Image.h>
#include <gammaengine/Font.h>

#include <Link.h>
#include <RawWifi.h>
#include <Socket.h>
#include "ControllerPi.h"
#include "Config.h"
#include "Stream.h"
#include "ui/Globals.h"

using namespace GE;

void bcm_host_init( void );

void segv_handler( int sig )
{
	void* array[16];
	size_t size;

	size = backtrace( array, 16 );

	fprintf( stderr, "Error: signal %d :\n", sig ); fflush(stderr);
	backtrace_symbols_fd( array, size, STDERR_FILENO );

// 	exit(0);
	pthread_exit(0);
}

int main( int ac, char** av )
{
	ControllerPi* controller = nullptr;
	Stream* stream = nullptr;
	Link* controller_link = nullptr;
	Link* stream_link = nullptr;

	if ( ac <= 1 ) {
		gDebug() << "FATAL ERROR : No config file specified !\n";
		return -1;
	}

	bcm_host_init();
	signal( SIGSEGV, segv_handler );

	Instance* instance = Instance::Create( "flight::control", 1, true, "framebuffer" );
	Font* font = new Font( "data/FreeMonoBold.ttf", 28 );
	Font* font_hud = new Font( "data/RobotoCondensed-Bold.ttf", 28, 0xFF000000 );

	Config* config = new Config( av[1] );
	config->Reload();

	Globals* globals = new Globals( instance, font );

	if ( config->string( "controller.link.link_type" ) == "Socket" ) {
		controller_link = new ::Socket( config->string( "controller.link.address", "192.168.32.1" ), config->integer( "controller.link.port", 2020 ) );
	} else if ( config->string( "controller.link.link_type" ) == "RawWifi" ) {
		controller_link = new RawWifi( config->string( "controller.link.device", "wlan0" ), config->integer( "controller.link.output_port", 0 ), config->integer( "controller.link.input_port", 1 ) );
	}

	if ( config->string( "stream.link.link_type" ) == "Socket" ) {
		stream_link = new ::Socket( config->string( "stream.link.address", "192.168.32.1" ), config->integer( "stream.link.port", 2020 ) );
	} else if ( config->string( "stream.link.link_type" ) == "RawWifi" ) {
		stream_link = new RawWifi( config->string( "stream.link.device", "wlan0" ), config->integer( "stream.link.output_port", 10 ), config->integer( "stream.link.input_port", 11 ) );
		dynamic_cast< RawWifi* >( stream_link )->setBlocking( config->boolean( "stream.link.blocking", true ) );
		dynamic_cast< RawWifi* >( stream_link )->setCECMode( config->string( "stream.link.cec_mode", "none" ) );
	}

	if ( controller_link ) {
		controller = new ControllerPi( controller_link );
	}
	if ( stream_link ) {
		stream = new Stream( controller, font_hud, stream_link, config->integer( "stream.width", 1920 ), config->integer( "stream.height", 1080 ), config->boolean( "stream.stereo", true ) );
		stream->setStereo( config->boolean( "stream.stereo", true ) );
		stream->setRenderHUD( config->boolean( "stream.hud", true ) );
	}

	globals->setStream( stream );
	globals->setController( controller );

	if ( config->boolean( "touchscreen.enabled", true ) ) {
		globals->setCurrentPage( "PageMain" );
		globals->Run();
	} else {
		while ( 1 ) {
			usleep( 1000 * 1000 );
		}
	}

	gDebug() << "Exiting\n";
	globals->instance()->Exit( 0 );

	return 0;
}
