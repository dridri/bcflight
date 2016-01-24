#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <pthread.h>

class Thread
{
public:
	Thread();
	virtual ~Thread();

	void Start();
	void Pause();
	void Stop();
	void Join();
	bool running();

protected:
	virtual bool run() = 0;

private:
	void ThreadEntry();
	bool mRunning;
	bool mIsRunning;
	bool mFinished;
	pthread_t mThread;
};


#endif // THREAD_H
