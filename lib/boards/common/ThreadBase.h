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

#ifndef THREADBASE_H
#define THREADBASE_H

#include <stdint.h>
#include <string>
#include <list>
#include <pthread.h>

using namespace std;

#define TIMEOUT -3
#define CONTINUE 0
typedef int32_t SyncReturn;


class ThreadBase
{
public:
	ThreadBase( const string& name = "_thead_" );
	virtual ~ThreadBase();

	void Start();
	void Pause();
	virtual void Stop();
	void Join();
	bool running() const;
	bool stopped() const;
	uint32_t frequency() const;

	void setFrequency( uint32_t hz );
	void setPriority( int p, int affinity = -1, bool priorityFifo = false );
	const string& name() const;

	virtual void Recover() = 0;

	static void setMainPriority( int p );
	static uint64_t GetTick();
	static float GetSeconds();
	static void StopAll();
	static void EnterCritical() {/* mCriticalMutex.lock();*/ }
	static void ExitCritical() {/* mCriticalMutex.unlock();*/ }

protected:
	string mName;
	uint32_t mID;
	bool mRunning;
	bool mStopped;
	bool mIsRunning;
	bool mFinished;
	int mPriority;
	int mSetPriority;
	int mAffinity;
	int mSetAffinity;
	bool mPriorityFifo;
	uint32_t mFrequency;
	uint64_t mFrequencyTick;

	static list< ThreadBase* >& threads();
};


#endif // THREADBASE_H

