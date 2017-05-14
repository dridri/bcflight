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

#include <wiringPi.h>
#include <softPwm.h>
#include "GPIO.h"


std::map< int, std::list<std::function<void()>> > GPIO::mInterrupts;
std::map< int, bool > GPIO::mFirstCall;


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


void GPIO::SetupInterrupt( int pin, GPIO::ISRMode mode, std::function<void()> fct )
{
	if ( mInterrupts.find( pin ) != mInterrupts.end() ) {
		mInterrupts.at( pin ).emplace_back( fct );
	} else {
		pinMode( pin, INPUT );
#define LINK_ISR(PIN) if(pin == PIN) { wiringPiISR( PIN, (int)mode, &GPIO::ISR_##PIN ); }
		LINK_ISR(1);
		LINK_ISR(2);
		LINK_ISR(3);
		LINK_ISR(4);
		LINK_ISR(5);
		LINK_ISR(6);
		LINK_ISR(7);
		LINK_ISR(8);
		LINK_ISR(9);
		LINK_ISR(10);
		LINK_ISR(11);
		LINK_ISR(12);
		LINK_ISR(13);
		LINK_ISR(14);
		LINK_ISR(15);
		LINK_ISR(16);
		LINK_ISR(17);
		LINK_ISR(18);
		LINK_ISR(19);
		LINK_ISR(20);
		LINK_ISR(21);
		LINK_ISR(22);
		LINK_ISR(23);
		LINK_ISR(24);
		LINK_ISR(25);
		LINK_ISR(26);
		LINK_ISR(27);
		LINK_ISR(28);
		LINK_ISR(29);
		LINK_ISR(30);
		LINK_ISR(31);
		LINK_ISR(32);
		std::list<std::function<void()>> lst;
		lst.emplace_back( fct );
		mInterrupts.emplace( std::make_pair( pin, lst ) );
	}
}


#define DECL_ISR(pin) \
void GPIO::ISR_##pin() \
{ \
	if ( mFirstCall.find(pin) == mFirstCall.end() ) { \
		mFirstCall.emplace( std::make_pair(pin, true) ); \
		piHiPri(99); \
	} \
	if ( mInterrupts.find(pin) != mInterrupts.end() ) { \
		for ( std::function<void()> fct : mInterrupts.at(pin) ) { \
			fct(); \
		} \
	} \
}

DECL_ISR(1);
DECL_ISR(2);
DECL_ISR(3);
DECL_ISR(4);
DECL_ISR(5);
DECL_ISR(6);
DECL_ISR(7);
DECL_ISR(8);
DECL_ISR(9);
DECL_ISR(10);
DECL_ISR(11);
DECL_ISR(12);
DECL_ISR(13);
DECL_ISR(14);
DECL_ISR(15);
DECL_ISR(16);
DECL_ISR(17);
DECL_ISR(18);
DECL_ISR(19);
DECL_ISR(20);
DECL_ISR(21);
DECL_ISR(22);
DECL_ISR(23);
DECL_ISR(24);
DECL_ISR(25);
DECL_ISR(26);
DECL_ISR(27);
DECL_ISR(28);
DECL_ISR(29);
DECL_ISR(30);
DECL_ISR(31);
DECL_ISR(32);
