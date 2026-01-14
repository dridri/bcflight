#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Thread.h>
#include <mutex>
#include "Config.h"
#include "Vector.h"

using namespace STD;


LUA_CLASS class BlackBox : public Thread
{
public:
	LUA_EXPORT BlackBox();
	~BlackBox();

	const uint32_t id() const;
	LUA_EXPORT void Enqueue( const string& data, const string& value );
	template<typename T, int n> void Enqueue( const string& data, const Vector<T, n>& v );
	void Enqueue( const string* data, const string* values, int n );
	void Enqueue( const char* data[], const char* values[], int n );
	void Enqueue( const string& data, const float* values, int n );
	template<typename T, int n> void Enqueue( const string* data, const Vector<T, n>* values, int m ) {
		if ( this == nullptr or not Thread::running() ) {
			return;
		}
		for ( int i = 0; i < m; i++ ) {
			Enqueue( data[i], values[i] );
		}
	}
	template<typename T> void Enqueue( const string& data, const std::vector<T>& v ) {
		if ( this == nullptr or not Thread::running() ) {
			return;
		}
		Enqueue( data, v.data(), (int)v.size() );
	}
	LUA_PROPERTY("enabled") void Start( bool enabled = true );

protected:
	virtual bool run();

	uint32_t mID;
	FILE* mFile;
#ifdef SYSTEM_NAME_Linux
	mutex mQueueMutex;
	uint32_t mSyncCounter;
	static const uint32_t MAX_QUEUE_SIZE = 1000; // seuil d'overrun
	static const uint32_t OVERRUN_DROP_COUNT = 100; // nombre d'entrées à sauter en cas d'overrun
#endif
	list< string > mQueue;
};

#endif // BLACKBOX_H
