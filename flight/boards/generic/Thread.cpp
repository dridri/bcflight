#include <unistd.h>
#include "Thread.h"

Thread::Thread()
	: mRunning( false )
	, mIsRunning( false )
	, mFinished( false )
{
	pthread_create( &mThread, nullptr, (void*(*)(void*))(void*)&Thread::ThreadEntry, this );
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


bool Thread::running()
{
	return mIsRunning;
}


void Thread::ThreadEntry()
{
	do {
		while ( !mRunning ) {
			mIsRunning = false;
			usleep( 1000 * 10 );
		}
		mIsRunning = true;
	} while ( run() );
	mIsRunning = false;
	mFinished = true;
}
