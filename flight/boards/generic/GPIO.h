#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <list>

class GPIO
{
public:
	typedef enum {
		Input,
		Output,
	} Mode;

	static void setMode( int pin, Mode mode );
	static void setPWM( int pin, int initialValue, int pwmRange );
	static void Write( int pin, bool en );
	static bool Read( int pin );

private:
};

#endif // GPIO_H
