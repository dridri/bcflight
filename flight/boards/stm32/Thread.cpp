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
#include "Board.h"
#include "Debug.h"

static list< Thread* > mThreads = list< Thread* >();
static Thread* mCurrentThread = nullptr;


Thread::Thread( const string& name )
	: ThreadBase( name )
{
	mThreads.emplace_back( this );
}


Thread::~Thread()
{
}


Thread* Thread::currentThread()
{
	return mCurrentThread;
}


void Thread::Join()
{
	uint64_t start_time = Board::GetTicks();

	while ( !mFinished ) {
		if ( Board::GetTicks() - start_time > 1000 * 1000 * 2 ) {
			gDebug() << "thread \"" << mName << "\" join timeout (2s), force killing\n";
			// TODO : kill thread
			break;
		}
		usleep( 1000 * 10 );
	}
}


void Thread::Recover()
{
	gDebug() << "\e[93mRecovering thread \"" << mName << "\"\e[0m\n";
	mRunning = false;
	mStopped = false;
	mIsRunning = false;
	mFinished = false;
	mPriority = 0;
	mAffinity = -1;

	// TODO ? hrmpf

	mRunning = true;
}


void Thread::ThreadEntry()
{
	uint32_t work_time_counter_last = Board::GetTicks();
	uint32_t total_work_time = 0;

	while ( 1 )
	{
		for ( Thread* thread : mThreads ) {
			if ( thread->mStopped ) {
				thread->mIsRunning = false;
				thread->mFinished = true;
				// TODO : delete from list ?
				continue;
			}
			if ( not thread->mRunning and not thread->mStopped ) {
				thread->mIsRunning = false;
				continue;
			}
			if ( thread->mRunning and not thread->mIsRunning ) {
				thread->mIsRunning = true;
			}
			if ( thread->mIsRunning and thread->mRunning ) {
				if ( Board::GetTicks() - thread->mFrequencyTick >= 1000000U / thread->mFrequency ) {
					uint32_t work_time_start = Board::GetTicks();
					thread->mFrequencyTick = work_time_start;
					mCurrentThread = thread;
					if ( thread->run() == false ) {
						thread->mStopped = true;
					}
					mCurrentThread = nullptr;
					total_work_time += Board::GetTicks() - work_time_start;
				}
			}
		}

		uint32_t curr = Board::GetTicks();
		if ( curr - work_time_counter_last >= 1000000U ) {
			gDebug() << "Work time : " << total_work_time << " us ( " << ( total_work_time / 10000U ) << "% )\n";
			work_time_counter_last = curr;
			total_work_time = 0;
		}
	}
}
