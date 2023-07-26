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
#include <fcntl.h>
#include <softPwm.h>
#include <string.h>
#include <string>
#include "GPIO.h"

std::map< int, std::list<std::pair<std::function<void()>,GPIO::ISRMode>> > GPIO::mInterrupts;

void GPIO::setMode( int pin, GPIO::Mode mode )
{
	if ( mode == Output ) {
		gpioSetMode( pin, PI_OUTPUT );
	} else {
		gpioSetMode( pin, PI_INPUT );
	}
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


void GPIO::SetupInterrupt( int pin, GPIO::ISRMode mode, std::function<void()> fct )
{
	if ( mInterrupts.find( pin ) != mInterrupts.end() ) {
		mInterrupts.at( pin ).emplace_back( std::make_pair( fct, mode ) );
	} else {
		std::list<std::pair<std::function<void()>,GPIO::ISRMode>> lst;
		lst.emplace_back( std::make_pair( fct, mode ) );
		mInterrupts.emplace( std::make_pair( pin, lst ) );

		system( ( "echo " + std::to_string( pin ) + " > /sys/class/gpio/export" ).c_str() );
		int32_t fd = open( ( "/sys/class/gpio/gpio" + std::to_string( pin ) + "/value" ).c_str(), O_RDWR );
		system( ( "echo both > /sys/class/gpio/gpio" + std::to_string( pin ) + "/edge" ).c_str() );
		int count = 0;
		char c = 0;
		ioctl( fd, FIONREAD, &count );
		for ( int i = 0; i < count; i++ ) {
			(void)read( fd, &c, 1 );
		}
		int32_t* argp = (int32_t*)malloc( sizeof(int32_t) * 2 );
		argp[0] = pin;
		argp[1] = fd;
		pthread_t thid;
		pthread_create( &thid, nullptr, (void*(*)(void*))&GPIO::ISR, argp );
	}
}


void* GPIO::ISR( void* argp )
{
	int32_t pin = ((int32_t*)argp)[0];
	int32_t fd = ((int32_t*)argp)[1];
	struct pollfd fds;
	char buffer[16];

	pthread_setname_np( pthread_self(), ( "GPIO::ISR_" + std::to_string( pin ) ).c_str() );
	struct sched_param sched;
	memset( &sched, 0, sizeof(sched) );
	sched.sched_priority = std::min( sched_get_priority_max( SCHED_RR ), 99 );
	sched_setscheduler( 0, SCHED_RR, &sched );
	usleep( 1000 );

	std::list<std::pair<std::function<void()>,GPIO::ISRMode>>& fcts = mInterrupts.at( pin );

	while ( true ) {
		fds.fd = fd;
		fds.events = POLLPRI;
		lseek( fd, 0, SEEK_SET );
		if ( poll( &fds, 1, -1 ) == 1 and read( fd, buffer, 2 ) > 0 ) {
			for ( std::pair<std::function<void()>,GPIO::ISRMode> fct : fcts ) {
				if ( buffer[0] == '1' and fct.second != Falling ) {
					fct.first();
				} else if ( buffer[0] == '0' and fct.second != Rising ) {
					fct.first();
				}
			}
		}
	}

	return nullptr;
}
