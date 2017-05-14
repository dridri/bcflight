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

#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <list>
#include <pthread.h>

class Thread
{
public:
	Thread( const std::string& name = "_thead_" );
	virtual ~Thread();

	void Start();
	void Pause();
	void Stop();
	void Join();
	bool running();
	void setPriority( int p, int affinity = -1 );
	static void setMainPriority( int p );

	static uint64_t GetTick();
	static float GetSeconds();
	static void StopAll();
	static void EnterCritical() {/* mCriticalMutex.lock();*/ }
	static void ExitCritical() {/* mCriticalMutex.unlock();*/ }

protected:
	virtual bool run() = 0;

private:
	void ThreadEntry();
	std::string mName;
	bool mRunning;
	bool mStopped;
	bool mIsRunning;
	bool mFinished;
	pthread_t mThread;
	int mPriority;
	int mSetPriority;
	int mAffinity;
	int mSetAffinity;
	static std::list< Thread* > mThreads;
};


template<typename T> class HookThread : public Thread
{
public:
	HookThread( const std::string& name, T* r, const std::function< bool( T* ) >& cb ) : Thread( name ), mT( r ), mCallback( cb ) {}
protected:
	virtual bool run() { return mCallback( mT ); }
private:
	T* mT;
	const std::function< bool( T* ) > mCallback;
};


#endif // THREAD_H
