/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <unistd.h>
#include <stdint.h>
#include "GPIO.h"

static map< int, list<pair<function<void()>,GPIO::ISRMode>> > mInterrupts;


GPIO_TypeDef* GPIO::_base( uint32_t pin )
{
	switch ( pin >> 4 ) {
		case 0:
			return GPIOA;
		case 1:
			return GPIOB;
		case 2:
			return GPIOC;
		case 3:
			return GPIOD;
		case 4:
			return GPIOE;
		case 5:
			return GPIOF;
		case 6:
			return GPIOG;
		case 7:
			return GPIOH;
		case 8:
			return GPIOI;
		default:
			break;
	}
	return nullptr;
}


void GPIO::setMode( uint32_t pin, GPIO::Mode mode, Pull pull, uint32_t alternate )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = 1 << ( pin & 0x0F );
	if ( alternate != 0 ) {
		if ( mode == OpenDrain ) {
			GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
		} else if ( pull != PullNone ) {
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		}
	} else {
		GPIO_InitStructure.Mode = ( mode == Input ? GPIO_MODE_INPUT : ( mode == OpenDrain ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP ) );
	}
	GPIO_InitStructure.Alternate = alternate;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull = ( pull == PullUp ? GPIO_PULLUP : ( pull == PullDown ? GPIO_PULLDOWN : GPIO_NOPULL ) );
	GPIO_TypeDef* b = _base( pin );
	if ( b != 0 ) {
		HAL_GPIO_Init( b, &GPIO_InitStructure );
	}
}


void GPIO::setPWM( uint32_t pin, int initialValue, int pwmRange )
{
}


void GPIO::Write( uint32_t pin, bool en )
{
	GPIO_TypeDef* b = _base( pin );
	if ( b != 0 ) {
		HAL_GPIO_WritePin( b, 1 << ( pin & 0x0F ), (GPIO_PinState)en );
	}
}


bool GPIO::Read( uint32_t pin )
{
	GPIO_TypeDef* b = _base( pin );
	if ( b != 0 ) {
		return HAL_GPIO_ReadPin( b, pin & 0x0F );
	}
	return 0;
}


void GPIO::SetupInterrupt( uint32_t pin, GPIO::ISRMode mode, function<void()> fct )
{
}
