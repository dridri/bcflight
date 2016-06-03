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
