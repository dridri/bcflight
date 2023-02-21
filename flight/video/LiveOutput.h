#ifndef LIVEOUTPUT_H
#define LIVEOUTPUT_H

#include "VideoEncoder.h"


LUA_CLASS class LiveOutput : public VideoEncoder
{
public:
	LUA_EXPORT LiveOutput();
	~LiveOutput();
	void Setup();
	void EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd = -1 );
};

#endif // LIVEOUTPUT_H
