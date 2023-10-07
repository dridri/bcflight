#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Thread.h>
#include <mutex>

using namespace STD;


LUA_CLASS class BlackBox : public Thread
{
public:
	LUA_EXPORT BlackBox();
	~BlackBox();

	const uint32_t id() const;
	LUA_EXPORT void Enqueue( const string& data, const string& value );
	LUA_PROPERTY("enabled") void Start( bool enabled ) {
		if ( enabled and not Thread::running() ) {
			Thread::Start();
		} else {
			Thread::Stop();
		}
	}

protected:
	virtual bool run();

	uint32_t mID;
	FILE* mFile;
#ifdef SYSTEM_NAME_Linux
	mutex mQueueMutex;
#endif
	list< string > mQueue;
};

#endif // BLACKBOX_H
