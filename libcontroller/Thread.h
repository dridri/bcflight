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
#include <pthread.h>

class Thread
{
public:
	Thread( const std::string& name );
	virtual ~Thread();
	void setPriority( int32_t prio );

	void Start();
	void Pause();
	void Stop();
	void Join();
	bool running();

	static uint64_t GetTick();

protected:
	virtual bool run() = 0;

private:
	static void sThreadEntry( void* argp );
	void ThreadEntry();
	bool mRunning;
	bool mIsRunning;
	bool mFinished;
	pthread_t mThread;
	int mPriority;
	int mSetPriority;
	bool mTerminate;
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
