#ifndef AVCODECENCODER_H
#define AVCODECENCODER_H

#include "VideoEncoder.h"

class AVCodec;
class AVCodecContext;


LUA_CLASS class AvcodecEncoder : public VideoEncoder
{
public:
	LUA_EXPORT typedef enum {
		FORMAT_H264,
		FORMAT_MJPEG
	} Format;

	LUA_EXPORT AvcodecEncoder();
	~AvcodecEncoder();
	virtual void EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd );

protected:
	virtual void Setup();

	LUA_PROPERTY("format") AvcodecEncoder::Format mFormat;
	LUA_PROPERTY("bitrate") int32_t mBitrate;
	LUA_PROPERTY("quality") int32_t mQuality;
	LUA_PROPERTY("width") int32_t mWidth;
	LUA_PROPERTY("height") int32_t mHeight;
	LUA_PROPERTY("framerate") int32_t mFramerate;

	bool mReady;
	const AVCodec* mCodec;
	AVCodecContext* mCodecContext;
};

#endif // AVCODECENCODER_H
