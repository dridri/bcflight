#ifndef LEDSTRIP_H
#define LEDSTRIP_H

#include <stdint.h>

class LEDStrip
{
public:
	LEDStrip();
	virtual ~LEDStrip();

	virtual void SetLEDColor( uint32_t led, uint8_t r, uint8_t g, uint8_t b ) = 0;
	virtual void Update() = 0;
};

#endif // LEDSTRIP_H
