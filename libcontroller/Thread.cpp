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
#ifdef WIN32
#include <windows.h>
#endif
#include "Thread.h"

std::mutex Thread::mCriticalMutex;
uint64_t Thread::mBaseTick = 0;

Thread::Thread( const std::string& name )
	: mName( name )
	, mRunning( false )
	, mIsRunning( false )
	, mFinished( false )
	, mPriority( 0 )
	, mSetPriority( 0 )
	, mAffinity( 0 )
	, mSetAffinity( 0 )
	, mTerminate( false )
{
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setstacksize( &attr, 16 * 1024 * 1024 );
	pthread_create( &mThread, &attr, (void*(*)(void*))&Thread::ThreadEntry, this );
#ifndef WIN32
	pthread_setname_np( mThread, name.substr( 0, 15 ).c_str() );
#endif
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


void Thread::setPriority( int32_t p, int affinity )
{
	mSetPriority = p;
#ifdef __linux__
	if ( affinity >= 0 and affinity < sysconf(_SC_NPROCESSORS_ONLN) ) {
		mSetAffinity = affinity;
	}
#endif
}


void Thread::sThreadEntry( void* argp )
{
	static_cast< Thread* >( argp )->ThreadEntry();
}


void Thread::ThreadEntry()
{
	do {
		while ( !mRunning ) {
			if ( mTerminate ) {
				return;
			}
			mIsRunning = false;
			usleep( 1000 * 100 );
		}
		mIsRunning = true;
		if ( mSetPriority != mPriority ) {
			mPriority = mSetPriority;
			printf( "Thread '%s' priority set to %d\n", mName.c_str(), mPriority );
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
		if ( mSetAffinity != mAffinity ) {
			mAffinity = mSetAffinity;
#ifdef __linux__
			printf( "Thread '%s' affinity set to %d\n", mName.c_str(), mAffinity );
			cpu_set_t cpuset;
			CPU_ZERO( &cpuset );
			CPU_SET( mAffinity, &cpuset );
			pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cpuset );
#endif
		}
	} while ( not mTerminate and run() );
	mIsRunning = false;
	mFinished = true;
}


uint64_t Thread::GetTick()
{
	uint64_t ret = 0;
#ifdef WIN32
	ret = timeGetTime();
#elif __APPLE__
	struct timeval cTime;
	gettimeofday( &cTime, 0 );
	ret = ( cTime.tv_sec * 1000 ) + ( cTime.tv_usec / 1000 );
#else
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	ret = now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif

	if ( mBaseTick == 0 ) {
		mBaseTick = ret;
	}
	return ret - mBaseTick;
}
