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

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <list>
#include <map>
#include <functional>

class GPIO
{
public:
	typedef enum {
		Input,
		Output,
	} Mode;
	typedef enum {
		PullOff = 0,
		PullDown = 1,
		PullUp = 2,
	} PUDMode;
	typedef enum {
		Falling = 1,
		Rising = 2,
		Both = 3,
	} ISRMode;

	static void setPUD( int pin, PUDMode mode );
	static void setMode( int pin, Mode mode );
	static void setPWM( int pin, int initialValue, int pwmRange );
	static void Write( int pin, bool en );
	static bool Read( int pin );
	static void SetupInterrupt( int pin, GPIO::ISRMode mode, std::function<void()> fct );

private:
	static std::map< int, std::list<std::pair<std::function<void()>,GPIO::ISRMode>> > mInterrupts;
	static void* ISR( void* argp );
};

#endif // GPIO_H
