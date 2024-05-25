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

#include <pigpio.h>
// #include <wiringPi.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <fcntl.h>
// #include <softPwm.h>
#include <Debug.h>
#include "GPIO.h"

map< int, list<pair<function<void()>,GPIO::ISRMode>> > GPIO::mInterrupts;
map< int, GPIO::ISR* > GPIO::mThreads;

void GPIO::setMode( int pin, GPIO::Mode mode )
{
	gpioSetMode( pin, mode );
	// if ( mode == Output ) {
	// 	gpioSetMode( pin, PI_OUTPUT );
	// } else {
	// 	gpioSetMode( pin, PI_INPUT );
	// }
}


void GPIO::setPUD( int pin, PUDMode mode )
{
	gpioSetPullUpDown( pin, mode );
}


void GPIO::setPWM( int pin, int initialValue, int pwmRange )
{
	// TODO : switch from wiringPi to pigpio
	// setMode( pin, Output );
	// softPwmCreate( pin, initialValue, pwmRange );
}


void GPIO::Write( int pin, bool en )
{
	gpioWrite( pin, en );
}


bool GPIO::Read( int pin )
{
	return gpioRead( pin );
}


// std::map< int, function<void()> > isrs;
// static bool ok = false;

void GPIO::SetupInterrupt( int pin, GPIO::ISRMode mode, function<void()> fct )
{
/*
	isrs[pin] = fct;
#define ISR( pin ) \
	wiringPiISR( pin, INT_EDGE_RISING, [](){ \
		if ( isrs[pin] ) { \
			isrs[pin](); \
		} \
	});

	if ( not ok ) {
		ISR( 22 );
		ISR( 27 );
		ok = true;
	}
*/
	if ( mInterrupts.find( pin ) != mInterrupts.end() ) {
		mInterrupts.at( pin ).emplace_back( make_pair( fct, mode ) );
	} else {
		list<pair<function<void()>,GPIO::ISRMode>> lst;
		lst.emplace_back( make_pair( fct, mode ) );
		mInterrupts.emplace( make_pair( pin, lst ) );

		system( ( "echo " + to_string( pin ) + " > /sys/class/gpio/export" ).c_str() );
		int32_t fd = open( ( "/sys/class/gpio/gpio" + to_string( pin ) + "/value" ).c_str(), O_RDWR );
		system( ( "echo both > /sys/class/gpio/gpio" + to_string( pin ) + "/edge" ).c_str() );
		int count = 0;
		char c = 0;
		ioctl( fd, FIONREAD, &count );
		for ( int i = 0; i < count; i++ ) {
			(void)read( fd, &c, 1 );
		}
		ISR* isr = new ISR( pin, fd );
		mThreads.emplace( make_pair( pin, isr ) );
		isr->Start();
		isr->setPriority( 99 );
	}
}


GPIO::ISR::ISR( int pin, int fd )
	: Thread( "GPIO::ISR_" + to_string(pin) )
	, mPin( pin )
	, mFD( fd )
	, mReady( false )
{
	mExitFD = eventfd( 0, EFD_NONBLOCK );
	if ( mExitFD < 0 ) {
		gDebug() << "Failed to create eventfd for GPIO ISR";
	}
}


bool GPIO::ISR::run()
{
	if ( not mReady ) {
		mReady = true;
		usleep( 1000 * 100 );
	}

	struct pollfd fds[2];
	char buffer[16];
	list<pair<function<void()>,GPIO::ISRMode>>& fcts = mInterrupts.at( mPin );

	fds[0].fd = mFD;
	fds[0].events = POLLPRI;
	fds[1].fd = mExitFD;
	fds[1].events = POLLIN;
	lseek( mFD, 0, SEEK_SET );
	int rpoll = poll( fds, 2, -1 );
	if ( rpoll >= 1 ) {
		if ( read( mExitFD, buffer, 16 ) > 0 and buffer[0] == 1 ) {
			return false;
		}
		if ( read( mFD, buffer, 2 ) > 0 ) {
			for ( pair<function<void()>,GPIO::ISRMode> fct : fcts ) {
				if ( buffer[0] == '1' and fct.second != Falling ) {
					fct.first();
				} else if ( buffer[0] == '0' and fct.second != Rising ) {
					fct.first();
				}
			}
		}
	}

	return true;
}


void GPIO::ISR::Stop()
{
	Thread::Stop();
	uint64_t val = 1;
	write( mExitFD, &val, sizeof(val) );
}
