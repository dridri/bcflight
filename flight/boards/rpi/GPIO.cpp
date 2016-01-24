#include <wiringPi.h>
#include <softPwm.h>
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
