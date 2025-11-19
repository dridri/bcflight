#ifndef MUTEX_H
#define MUTEX_H

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
};

#endif // MUTEX_H
