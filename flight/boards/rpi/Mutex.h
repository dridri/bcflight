#ifndef MUTEX_H
#define MUTEX_H

#include <mutex>

class Mutex
{
public:
	Mutex();
	~Mutex();
	void Lock();
	void Unlock();
	void lock();
	void unlock();

protected:
	std::mutex mMutex;
};

#endif // MUTEX_H
