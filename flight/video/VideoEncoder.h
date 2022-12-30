#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <stdint.h>
#include "Lua.h"
#include "Recorder.h"

class VideoEncoder
{
public:
	VideoEncoder();
	~VideoEncoder();
	virtual void Setup() = 0;
	virtual void EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd = -1 ) = 0;

protected:
	LUA_PROPERTY("recorder") Recorder* mRecorder;
	uint32_t mRecorderTrackId;
};

#endif // VIDEOENCODER_H
