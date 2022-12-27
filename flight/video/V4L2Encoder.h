#ifndef V4L2ENCODER_H
#define V4L2ENCODER_H

#include "Lua.h"
#include "VideoEncoder.h"

LUA_CLASS class V4L2Encoder : public VideoEncoder
{
public:
	LUA_EXPORT V4L2Encoder();
	~V4L2Encoder();
	void Setup();

protected:
	LUA_PROPERTY("video_device") std::string mVideoDevice;
	LUA_PROPERTY("bitrate") int32_t mBitrate;
	LUA_PROPERTY("width") int32_t mWidth;
	LUA_PROPERTY("height") int32_t mHeight;
	LUA_PROPERTY("framerate") int32_t mFramerate;

	int mFD;
};

#endif // V4L2ENCODER_H
