#include "Mutex.h"

Mutex::Mutex()
{
}


Mutex::~Mutex()
{
}


void Mutex::Lock()
{
	mMutex.lock();
}


void Mutex::Unlock()
{
	mMutex.unlock();
}


void Mutex::lock()
{
	mMutex.lock();
}


void Mutex::unlock()
{
	mMutex.unlock();
}

