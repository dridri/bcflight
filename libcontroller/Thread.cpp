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
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <signal.h>
#include "Thread.h"

Thread::Thread( const std::string& name )
	: mRunning( false )
	, mIsRunning( false )
	, mFinished( false )
	, mPriority( 0 )
	, mSetPriority( 0 )
	, mTerminate( false )
{
	pthread_create( &mThread, nullptr, (void*(*)(void*))&Thread::ThreadEntry, this );
	pthread_setname_np( mThread, name.substr( 0, 15 ).c_str() );
}


Thread::~Thread()
{
}


void Thread::Start()
{
	mRunning = true;
}


void Thread::Stop()
{
	mTerminate = true;
}


void Thread::Pause()
{
	mRunning = false;
}


void Thread::Join()
{
	while ( !mFinished ) {
		usleep( 1000 * 10 );
	}
}


bool Thread::running()
{
	if ( pthread_kill( mThread, 0 ) == ESRCH ) {
		mIsRunning = false;
	}
	return mIsRunning;
}


void Thread::setPriority( int32_t p )
{
	mSetPriority = p;
}


void Thread::ThreadEntry()
{
	do {
		while ( !mRunning ) {
			mIsRunning = false;
			usleep( 1000 * 100 );
		}
		mIsRunning = true;
		if ( mSetPriority != mPriority ) {
			mPriority = mSetPriority;
#ifdef __linux__
			struct sched_param sched;
			memset( &sched, 0, sizeof(sched) );
			if ( mPriority > sched_get_priority_max( SCHED_RR ) ) {
				sched.sched_priority = sched_get_priority_max( SCHED_RR );
			} else{ 
				sched.sched_priority = mPriority;
			}
			sched_setscheduler( 0, SCHED_RR, &sched );
#endif
		}
	} while ( not mTerminate and run() );
	mIsRunning = false;
	mFinished = true;
}


uint64_t Thread::GetTick()
{
#ifdef WIN32
	return timeGetTime();
#elif __APPLE__
	struct timeval cTime;
	gettimeofday( &cTime, 0 );
	return ( cTime.tv_sec * 1000 ) + ( cTime.tv_usec / 1000 );
#else
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif
}
