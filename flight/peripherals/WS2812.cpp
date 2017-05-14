#include <string.h>
#include "WS2812.h"

WS2812::WS2812( uint8_t gpio_pin, uint32_t leds_count )
	: mGPIOPin( gpio_pin )
	, mLedsCount( leds_count )
{
/*
	mPWM = new PWM( 14, 10 * 1000 * 1000, 13 * 24, 1, &gpio_pin, 1 );
// 	mPWM = new PWM( 14, 1 * 1000 * 1000, 13 * 24 + 600, 1, &gpio_pin, 1 );
*/
	mLedsColors = new rgb_t[ mLedsCount ];
	memset( mLedsColors, 0, sizeof(rgb_t) * mLedsCount );
}


WS2812::~WS2812()
{
}


void WS2812::SetLEDColor( uint32_t led, uint8_t r, uint8_t g, uint8_t b )
{
	mLedsColors[led].r = r;
	mLedsColors[led].g = g;
	mLedsColors[led].b = b;
}


void WS2812::Update()
{
	int32_t i = 0;
	uint32_t j = 0;
	uint8_t buffer[13 * 24];
	memset( buffer, 0, sizeof(buffer) );

	for ( i = 7; i >= 0; i-- ) {
		if ( ( mLedsColors[0].r >> i ) & 0x1 ) {
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
		} else {
			buffer[j++] = 1;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
		}
	}
	for ( i = 7; i >= 0; i-- ) {
		if ( ( mLedsColors[0].g >> i ) & 0x1 ) {
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
		} else {
			buffer[j++] = 1;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
		}
	}
	for ( i = 7; i >= 0; i-- ) {
		if ( ( mLedsColors[0].b >> i ) & 0x1 ) {
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 1;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
		} else {
			buffer[j++] = 1;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
			buffer[j++] = 0;
		}
	}

// 	mPWM->SetPWMBuffer( mGPIOPin, buffer, sizeof(buffer) );
// 	mPWM->SetPWMus( mGPIOPin, 200 );
// 	mPWM->Update();
}
