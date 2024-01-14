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
#include "ThreadBase.h"
#include "Board.h"
#include "Debug.h"

static list< ThreadBase* > mThreads;


list< ThreadBase* >& ThreadBase::threads()
{
	return mThreads;
}

ThreadBase::ThreadBase( const string& name )
	: mName( name )
	, mID( 0 )
	, mRunning( false )
	, mStopped( false )
	, mIsRunning( false )
	, mFinished( false )
	, mPriority( 0 )
	, mSetPriority( 0 )
	, mAffinity( -1 )
	, mSetAffinity( -1 )
	, mPriorityFifo( false )
	, mFrequency( 0 )
	, mFrequencyTick( 0 )
{
	mThreads.emplace_back( this );
	gDebug() << "New thread : " << name;
}


ThreadBase::~ThreadBase()
{
}


const string& ThreadBase::name() const
{
	return mName;
}


void ThreadBase::StopAll()
{
	gDebug() << "Stopping all threads !";

	// Stop ISRs first
	for ( ThreadBase* thread : mThreads ) {
		if ( not thread->mStopped and thread->name().find( "ISR" ) != string::npos ) {
			gDebug() << "Stopping ISR thread " << thread->mName;
			static_cast<Thread*>(thread)->Stop();
			static_cast<Thread*>(thread)->Join();
		}
	}

	for ( ThreadBase* thread : mThreads ) {
		if ( not thread->mStopped ) {
			gDebug() << "Stopping thread " << thread->mName;
			static_cast<Thread*>(thread)->Stop();
			static_cast<Thread*>(thread)->Join();
		}
	}
	gDebug() << "Stopped all threads !";
}


uint64_t ThreadBase::GetTick()
{
	return Board::GetTicks();
}


float ThreadBase::GetSeconds()
{
	return (float)( GetTick() / 1000 ) / 1000.0f;
}


void ThreadBase::Start()
{
	mStopped = false;
	mFinished = false;
	mRunning = true;
}


void ThreadBase::Pause()
{
	mRunning = false;
}


void ThreadBase::Stop()
{
	mStopped = true;
}


void ThreadBase::Join()
{
	while ( !mFinished ) {
		usleep( 1000 * 10 );
	}
}


bool ThreadBase::running() const
{
	return mIsRunning;
}


bool ThreadBase::stopped() const
{
	return mStopped;
}


uint32_t ThreadBase::frequency() const
{
	return mFrequency;
}



void ThreadBase::setMainPriority( int p )
{
// 	piHiPri( p );
}


void ThreadBase::setPriority( int p, int affinity, bool priorityFifo )
{
	mSetPriority = p;
	if ( affinity >= 0 ) {
		mSetAffinity = affinity;
		mPriorityFifo = priorityFifo;
	}
}


void ThreadBase::setFrequency( uint32_t hz )
{
	mFrequency = hz;
}
