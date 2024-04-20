#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Thread.h>
#include <mutex>
#include "Config.h"

using namespace STD;


LUA_CLASS class BlackBox : public Thread
{
public:
	LUA_EXPORT BlackBox();
	~BlackBox();

	const uint32_t id() const;
	LUA_EXPORT void Enqueue( const string& data, const string& value );
	void Enqueue( const string* data, const string* values, int n );
	void Enqueue( const char* data[], const char* values[], int n );
	LUA_PROPERTY("enabled") void Start( bool enabled = true );

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
