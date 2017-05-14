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
		Falling = 1,
		Rising = 2,
		Both = 3,
	} ISRMode;

	static void setMode( int pin, Mode mode );
	static void setPWM( int pin, int initialValue, int pwmRange );
	static void Write( int pin, bool en );
	static bool Read( int pin );
	static void SetupInterrupt( int pin, GPIO::ISRMode mode, std::function<void()> fct );

private:
	static std::map< int, bool > mFirstCall;
	static std::map< int, std::list<std::function<void()>> > mInterrupts;
	static void ISR_1();
	static void ISR_2();
	static void ISR_3();
	static void ISR_4();
	static void ISR_5();
	static void ISR_6();
	static void ISR_7();
	static void ISR_8();
	static void ISR_9();
	static void ISR_10();
	static void ISR_11();
	static void ISR_12();
	static void ISR_13();
	static void ISR_14();
	static void ISR_15();
	static void ISR_16();
	static void ISR_17();
	static void ISR_18();
	static void ISR_19();
	static void ISR_20();
	static void ISR_21();
	static void ISR_22();
	static void ISR_23();
	static void ISR_24();
	static void ISR_25();
	static void ISR_26();
	static void ISR_27();
	static void ISR_28();
	static void ISR_29();
	static void ISR_30();
	static void ISR_31();
	static void ISR_32();
};

#endif // GPIO_H
