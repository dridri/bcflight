#ifndef WS2812_H
#define WS2812_H

#include "LEDStrip.h"
#include <PWM.h>

class WS2812 : public LEDStrip
{
public:
	WS2812( uint8_t gpio_pin, uint32_t leds_count );
	~WS2812();

	void SetLEDColor( uint32_t led, uint8_t r, uint8_t g, uint8_t b );
	void Update();

private:
	typedef struct rgb_s {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	} rgb_t;

	uint8_t mGPIOPin;
	uint32_t mLedsCount;
	rgb_t* mLedsColors;
	PWM* mPWM;
};

#endif // WS2812_H
