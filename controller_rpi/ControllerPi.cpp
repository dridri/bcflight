#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <wiringPi.h>
#include <gammaengine/Time.h>
#include <gammaengine/Debug.h>
#include "ControllerPi.h"

ControllerPi::ControllerPi( Link* link )
	: Controller( link )
{
	wiringPiSetup();
	pinModeAlt( 21, INPUT );
	pinModeAlt( 22, INPUT );
	pinMode( 21, INPUT );
	pinMode( 22, INPUT );
	pinMode( 23, INPUT );
	pinMode( 24, INPUT );
	pinMode( 25, INPUT );
}


ControllerPi::~ControllerPi()
{
}


int8_t ControllerPi::ReadSwitch( uint32_t id )
{
	return not digitalRead( 21 + id );
}
