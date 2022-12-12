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
#include <wiringPi.h>
#include <iostream>
#include "Thread.h"
#include "Board.h"
#include "Debug.h"


Thread::Thread( const string& name )
	: ThreadBase( name )
	, mSpawned( false )
{
	mID = mThread;
}


Thread::~Thread()
{
}


void Thread::Start()
{
	if ( not mSpawned ) {
		pthread_create( &mThread, nullptr, (void*(*)(void*))&Thread::ThreadEntry, this );
		pthread_setname_np( mThread, mName.substr( 0, 15 ).c_str() );
		mSpawned = true;
	}
	mRunning = true;
}


Thread* Thread::currentThread()
{
	for ( ThreadBase* th : threads() ) {
		if ( static_cast<Thread*>(th)->mThread == pthread_self() ) {
			return static_cast<Thread*>(th);
		}
	}
	return nullptr;
}


void Thread::Join()
{
	uint64_t start_time = Board::GetTicks();

	while ( !mFinished ) {
		if ( Board::GetTicks() - start_time > 1000 * 1000 * 2 ) {
			gDebug() << "thread " << mName << " join timeout (2s), force killing";
		//	pthread_cancel( mThread );
			break;
		}
		usleep( 1000 * 10 );
	}
}


void Thread::Recover()
{
	gDebug() << "\e[93mRecovering thread " << mName << "\e[0m";
	mRunning = false;
	mStopped = false;
	mIsRunning = false;
	mFinished = false;
	mPriority = 0;
	mAffinity = -1;

	pthread_create( &mThread, nullptr, (void*(*)(void*))&Thread::ThreadEntry, this );
	pthread_setname_np( mThread, mName.substr( 0, 15 ).c_str() );
	mID = mThread;

	mRunning = true;
}


void Thread::ThreadEntry()
{
	do {
		while ( not mRunning and not mStopped ) {
			mIsRunning = false;
			usleep( 1000 * 100 );
		}
		if ( mRunning ) {
			mIsRunning = true;
			if ( mSetPriority != mPriority ) {
				mPriority = mSetPriority;
				piHiPri( mPriority );
			}
			if ( mSetAffinity >= 0 and mSetAffinity != mAffinity ) {
				mAffinity = mSetAffinity;
				cpu_set_t cpuset;
				CPU_ZERO( &cpuset );
				CPU_SET( mAffinity, &cpuset );
				pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cpuset );
			}
		}
		if ( mFrequency > 0 ) {
			uint32_t div = 1000000L / mFrequency;
			uint32_t drift = mFrequency >= 1000 ? -50 : -250;
			mFrequencyTick = Board::WaitTick( div, mFrequencyTick, drift );
		}
	} while ( not mStopped and run() );
	mIsRunning = false;
	mFinished = true;
}


void Thread::msleep( uint32_t ms )
{
	::usleep( ms * 1000 );
}


void Thread::usleep( uint32_t us )
{
	::usleep( us );
}
