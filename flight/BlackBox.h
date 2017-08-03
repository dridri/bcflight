#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Thread.h>
#include <mutex>

class BlackBox : public Thread
{
public:
	BlackBox();
	~BlackBox();

	const uint32_t id() const;
	void Enqueue( const std::string& data, const std::string& value );

protected:
	virtual bool run();

	uint32_t mID;
	FILE* mFile;
	std::mutex mQueueMutex;
	std::list< std::string > mQueue;
};

#endif // BLACKBOX_H
