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

	fprintf( stderr, "Error: signal %d :\n", sig );
	backtrace_symbols_fd( array, size, STDERR_FILENO );
}

int main( int ac, char** av )
{
// 	chdir( "/root/ge/" );
// 	system( "echo -n \"pwd : \" && pwd" );
// 	daemon( 0, 1 );
// 	system( "ifconfig wlan0 down && iw dev wlan0 set monitor otherbss fcsfail && ifconfig wlan0 up && iwconfig wlan0 channel 13 && iw dev wlan0 set bitrates ht-mcs-2.4 3 && iwconfig wlan0 rate 26M && iw dev wlan0 set txpower fixed 30000" );

	bcm_host_init();
	signal(SIGSEGV, segv_handler);

	Instance* instance = Instance::Create( "flight::control", 1, true, "framebuffer" );
	Font* font = new Font( "data/FreeMonoBold.ttf", 28 );
	Font* font_hud = new Font( "data/RobotoCondensed-Bold.ttf", 28, 0xFF000000 );

	Globals* globals = new Globals( instance, font );

	Link* controller_link = new RawWifi( "wlan0", 0, 0 );
// 	Link* stream_link = new RawWifi( "wlan0", 1, 1 );

	Controller* controller = new ControllerPi( controller_link );
	Stream* stream = new Stream( controller, font_hud, "192.168.32.1", 2021 );

	globals->setStream( stream );
	globals->setController( controller );
	globals->setCurrentPage( "PageMain" );
	globals->Run();

	gDebug() << "Exiting\n";
	globals->instance()->Exit( 0 );
	return 0;
}
