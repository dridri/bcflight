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

std::list< Thread* > Thread::mThreads;

Thread::Thread( const std::string& name )
	: mName( name )
	, mRunning( false )
	, mStopped( false )
	, mIsRunning( false )
	, mFinished( false )
	, mPriority( 0 )
	, mSetPriority( 0 )
	, mAffinity( -1 )
	, mSetAffinity( -1 )
{
	mThreads.emplace_back( this );
	pthread_create( &mThread, nullptr, (void*(*)(void*))&Thread::ThreadEntry, this );
	pthread_setname_np( mThread, name.substr( 0, 15 ).c_str() );
}


Thread::~Thread()
{
}


void Thread::StopAll()
{
	std::cerr << "Stopping all threads !\n";
	for ( Thread* thread : mThreads ) {
		std::cerr << "Stopping thread \"" << thread->mName << "\"\n";
		thread->Stop();
		thread->Join();
	}
	std::cerr << "Stopped all threads !\n";
}


uint64_t Thread::GetTick()
{
	return Board::GetTicks();
}


float Thread::GetSeconds()
{
	return (float)( GetTick() / 1000 ) / 1000.0f;
}


void Thread::Start()
{
	mRunning = true;
}


void Thread::Pause()
{
	mRunning = false;
}


void Thread::Stop()
{
	mStopped = true;
}


void Thread::Join()
{
	while ( !mFinished ) {
		usleep( 1000 * 10 );
	}
}


bool Thread::running()
{
	return mIsRunning;
}


void Thread::setMainPriority( int p )
{
	piHiPri( p );
}


void Thread::setPriority( int p, int affinity )
{
	mSetPriority = p;
	if ( affinity >= 0 and affinity < sysconf(_SC_NPROCESSORS_ONLN) ) {
		mSetAffinity = affinity;
	}
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
	} while ( not mStopped and run() );
	mIsRunning = false;
	mFinished = true;
}
