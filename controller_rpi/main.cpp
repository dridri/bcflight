#include <gammaengine/Instance.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include <gammaengine/Window.h>
#include <gammaengine/FramebufferWindow.h>
#include <gammaengine/Input.h>
#include <gammaengine/Renderer2D.h>
#include <gammaengine/Image.h>
#include <gammaengine/Font.h>

#include "Controller.h"
#include "Stream.h"
#include "ui/Globals.h"

using namespace GE;

void bcm_host_init( void );

int main( int ac, char** av )
{
// 	system( "ifconfig wlan0 down && iw dev wlan0 set monitor otherbss fcsfail && ifconfig wlan0 up && iwconfig wlan0 channel 13 && iw dev wlan0 set bitrates ht-mcs-2.4 3 && iwconfig wlan0 rate 26M && iw dev wlan0 set txpower fixed 30000" );

	bcm_host_init();
	Instance* instance = Instance::Create( "flight::control", 1, true, "framebuffer" );
	Font* font = new Font( "data/FreeMonoBold.ttf", 32 );

	Globals* globals = new Globals( instance, font );

	Controller* controller = new Controller( "192.168.32.1", 2020 );
	Stream* stream = new Stream( font, "192.168.32.1", 2021 );
// 	Stream* stream = new Stream( font, "192.168.32.255", 2021 );

	globals->setController( controller );
	globals->setCurrentPage( "PageMain" );
// 	globals->setCurrentPage( "PageCalibrate" );
	globals->Run();

	gDebug() << "Exiting\n";
	globals->instance()->Exit( 0 );
	return 0;
}
