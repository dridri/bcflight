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
#include "Thread.h"

// This fil contains a pthread implementation (same as used in 'rpi' board)

Thread::Thread( const string& name )
	: mRunning( false )
	, mIsRunning( false )
	, mFinished( false )
	, mPriority( 0 )
	, mSetPriority( 0 )
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


bool Thread::running() const
{
	return mIsRunning;
}


void Thread::setMainPriority( int p )
{
	// set priority of main thread here
}


void Thread::setPriority( int p, int affinity )
{
	mSetPriority = p;
}


void Thread::ThreadEntry()
{
	do {
		while ( !mRunning ) {
			mIsRunning = false;
			usleep( 1000 * 10 );
		}
		mIsRunning = true;
		if ( mSetPriority != mPriority ) {
			mPriority = mSetPriority;
			// set priority here
		}
	} while ( run() ); // A thread should return 'true' to keep looping on it, or 'false' for one-shot mode or to exit
	mIsRunning = false;
	mFinished = true;
}
