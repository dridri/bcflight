#include <unistd.h>
#include <wiringPi.h>
#include "Thread.h"

Thread::Thread( const std::string& name )
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


bool Thread::running()
{
	return mIsRunning;
}


void Thread::setMainPriority( int p )
{
	piHiPri( p );
}


void Thread::setPriority( int p )
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
			piHiPri( mPriority );
		}
	} while ( run() );
	mIsRunning = false;
	mFinished = true;
}
