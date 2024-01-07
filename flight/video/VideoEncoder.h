#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <stdint.h>
#include "Lua.h"
#include "Link.h"
#include "Recorder.h"

class VideoEncoder
{
public:
	VideoEncoder();
	~VideoEncoder();
	virtual void Setup() = 0;
	virtual void EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd = -1 ) = 0;
	virtual void SetInputResolution( int width, int height );

protected:
	LUA_PROPERTY("recorder") Recorder* mRecorder;
	LUA_PROPERTY("link") Link* mLink;
	uint32_t mRecorderTrackId;
	uint32_t mInputWidth;
	uint32_t mInputHeight;
};

#endif // VIDEOENCODER_H
