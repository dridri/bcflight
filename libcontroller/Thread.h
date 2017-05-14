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

#include <mutex>
#include <pthread.h>

class Thread
{
public:
	Thread( const std::string& name );
	virtual ~Thread();
	void setPriority( int32_t prio, int affinity = -1 );

	void Start();
	void Pause();
	void Stop();
	void Join();
	bool running();

	static uint64_t GetTick();
	static float GetSeconds();
	static void EnterCritical() {/* mCriticalMutex.lock();*/ }
	static void ExitCritical() {/* mCriticalMutex.unlock();*/ }

protected:
	virtual bool run() = 0;

private:
	static void sThreadEntry( void* argp );
	void ThreadEntry();
	std::string mName;
	bool mRunning;
	bool mIsRunning;
	bool mFinished;
	pthread_t mThread;
	int mPriority;
	int mSetPriority;
	int mAffinity;
	int mSetAffinity;
	bool mTerminate;

	static std::mutex mCriticalMutex;
	static uint64_t mBaseTick;
};


template<typename T> class HookThread : public ::Thread
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
