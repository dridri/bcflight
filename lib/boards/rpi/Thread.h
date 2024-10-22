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
#include <functional>
#include <pthread.h>
#include "../common/ThreadBase.h"

class Thread : public ThreadBase
{
public:
	Thread( const string& name = "_thead_" );
	virtual ~Thread();

	void Start();
	void Join();
	void Recover();

	static Thread* currentThread();
	static void msleep( uint32_t ms );
	static void usleep( uint32_t us );

protected:
	virtual bool run() = 0;
	void setName( const string& name );

private:
	void ThreadEntry();
	pthread_t mThread;
	bool mSpawned;
};


template<typename T> class HookThread : public Thread
{
public:
	HookThread( const string& name, T* r, const function< bool( T* ) >& cb ) : Thread( name ), mT( r ), mCallback( cb ) {}
protected:
	virtual bool run() { return mCallback( mT ); }
private:
	T* mT;
	const function< bool( T* ) > mCallback;
};


#endif // THREAD_H
