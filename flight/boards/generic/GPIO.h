#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <list>
#include <functional>

class GPIO
{
public:
	typedef enum {
		Input,
		Output,
	} Mode;
	typedef enum {
		Falling,
		Rising,
		Both,
	} ISRMode;

	static void setMode( int pin, Mode mode );
	static void setPWM( int pin, int initialValue, int pwmRange );
	static void Write( int pin, bool en );
	static bool Read( int pin );
	static void RegisterInterrupt( int pin, GPIO::ISRMode mode, void (*cb)(void*) );

private:
};

#endif // GPIO_H
