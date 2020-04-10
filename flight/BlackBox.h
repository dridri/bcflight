#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Thread.h>
#include <mutex>

using namespace STD;


class BlackBox : public Thread
{
public:
	BlackBox();
	~BlackBox();

	const uint32_t id() const;
	void Enqueue( const string& data, const string& value );

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
