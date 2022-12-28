#ifndef LIVEOUTPUT_H
#define LIVEOUTPUT_H

#include "VideoEncoder.h"


class LiveOutput : public VideoEncoder
{
public:
	LiveOutput();
	~LiveOutput();
	void EnqueueBuffer( int size, void* mem, int64_t timestamp_us, int fd );
};

#endif // LIVEOUTPUT_H
