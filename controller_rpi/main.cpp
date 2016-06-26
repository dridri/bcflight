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

	exit(0);
}

int main( int ac, char** av )
{

	bcm_host_init();
	signal( SIGSEGV, segv_handler );

	Instance* instance = Instance::Create( "flight::control", 1, true, "framebuffer" );
	Font* font = new Font( "data/FreeMonoBold.ttf", 28 );
	Font* font_hud = new Font( "data/RobotoCondensed-Bold.ttf", 28, 0xFF000000 );

	Globals* globals = new Globals( instance, font );

// 	Link* controller_link = new ::Socket( "192.168.32.1", 2020 );
	Link* controller_link = new RawWifi( "wlan0", 0, 1 );
// 	Link* stream_link = new ::Socket( "192.168.32.1", 2021 );
	Link* stream_link = new RawWifi( "wlan0", 10, 11 );

	ControllerPi* controller = new ControllerPi( controller_link );
	Stream* stream = new Stream( controller, font_hud, stream_link );

	globals->setStream( nullptr );
	globals->setStream( stream );
	globals->setController( controller );
	globals->setCurrentPage( "PageMain" );
	globals->Run();

	gDebug() << "Exiting\n";
	globals->instance()->Exit( 0 );

	return 0;
}
