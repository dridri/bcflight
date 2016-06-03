#include <wiringPi.h>
#include <softPwm.h>
#include <Debug.h>
#include "GPIO.h"


void GPIO::setMode( int pin, GPIO::Mode mode )
{
	if ( mode == Output ) {
		pinMode( pin, OUTPUT );
	} else {
		pinMode( pin, INPUT );
	}
}


void GPIO::setPWM( int pin, int initialValue, int pwmRange )
{
	setMode( pin, Output );
	softPwmCreate( pin, initialValue, pwmRange );
}


void GPIO::Write( int pin, bool en )
{
	digitalWrite( pin, en );
}


bool GPIO::Read( int pin )
{
	return digitalRead( pin );
}


void GPIO::SetupInterrupt( int pin, GPIO::ISRMode mode )
{
	char command[256] = "";
	std::string smode = "falling";

	if ( mode == GPIO::Rising ) {
		smode = "rising";
	} else if ( mode == GPIO::Both ) {
		smode = "both";
	}

	sprintf( command, "gpio edge %d %s", pin, smode.c_str() );
	system( command );
}
