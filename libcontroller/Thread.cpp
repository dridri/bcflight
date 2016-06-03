#include <unistd.h>
#include <sched.h>
#include <string.h>
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
			usleep( 1000 * 10 );
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
