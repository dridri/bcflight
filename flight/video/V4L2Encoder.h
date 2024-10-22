#ifndef V4L2ENCODER_H
#define V4L2ENCODER_H

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "Lua.h"
#include "VideoEncoder.h"
#include "Thread.h"

LUA_CLASS class V4L2Encoder : public VideoEncoder, protected Thread
{
public:
	LUA_EXPORT typedef enum {
		FORMAT_H264,
		FORMAT_MJPEG
	} Format;

	LUA_EXPORT V4L2Encoder();
	~V4L2Encoder();
	virtual void EnqueueBuffer( size_t size, void* mem, int64_t timestamp_us, int fd = -1 );

protected:
	void Setup();
	void SetupH264();
	void SetupMJPEG();
	virtual bool run();

	typedef struct {
		void* mem;
		size_t size;
	} BufferDescription;

	typedef struct {
		void* mem;
		size_t bytes_used;
		size_t length;
		unsigned int index;
		bool keyframe;
		int64_t timestamp_us;
	} OutputItem;

	LUA_PROPERTY("video_device") std::string mVideoDevice;
	LUA_PROPERTY("format") V4L2Encoder::Format mFormat;
	LUA_PROPERTY("bitrate") int32_t mBitrate;
	LUA_PROPERTY("quality") int32_t mQuality;
	LUA_PROPERTY("width") int32_t mWidth;
	LUA_PROPERTY("height") int32_t mHeight;
	LUA_PROPERTY("framerate") int32_t mFramerate;

	bool mReady;
	int mFD;
	std::queue<int> mInputBuffersAvailable;
	std::mutex mInputBuffersAvailableMutex;
	int32_t mOutputBuffersCount;
	std::vector<BufferDescription> mOutputBuffers;
	std::queue<OutputItem> mOutputQueue;
	std::mutex mOutputMutex;
	std::condition_variable mOutputCondVar;
};

#endif // V4L2ENCODER_H
