#ifndef RECORDERAVFORMAT_H
#define RECORDERAVFORMAT_H

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
};
#include "Recorder.h"
#include "Thread.h"

LUA_CLASS class RecorderAvformat : public Recorder, protected Thread
{
public:
	LUA_EXPORT RecorderAvformat();
	~RecorderAvformat();
	LUA_EXPORT virtual bool recording();
	LUA_EXPORT virtual void Stop();
	LUA_EXPORT virtual void Start();

	virtual uint32_t AddAudioTrack( uint32_t channels, uint32_t sample_rate, const std::string& extension );
	virtual uint32_t AddVideoTrack( uint32_t width, uint32_t height, uint32_t average_fps, const std::string& extension );
	virtual void WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen );

	virtual void WriteGyro( uint64_t record_time_us, const Vector3f& gyro, const Vector3f& accel = Vector3f() );

protected:
	typedef struct {
		uint32_t id;
		AVStream* stream;
		TrackType type;
		// video
		uint32_t width;
		uint32_t height;
		uint32_t average_fps;
		// audio
		uint32_t channels;
		uint32_t sample_rate;
	} Track;

	typedef struct {
		Track* track;
		uint64_t record_time_us;
		uint8_t* buf;
		uint32_t buflen;
		bool keyframe;
	} PendingSample;

	typedef struct {
		uint64_t t;
		float gx;
		float gy;
		float gz;
		float ax;
		float ay;
		float az;
	} PendingGyro;

	bool run();
	void WriteSample( PendingSample* sample );

	LUA_PROPERTY("base_directory") string mBaseDirectory;
	uint32_t mRecordId;
	uint64_t mRecordStartTick;
	uint64_t mRecordStartSystemTick;

	std::vector< Track* > mTracks;
	std::mutex mWriteMutex;
	std::list< PendingSample* > mPendingSamples;
	AVFormatContext* mOutputContext;
	bool mMuxerReady;
	bool mStopWrite;
	uint64_t mWriteTicks;

	std::vector< uint8_t > mVideoHeader;

	FILE* mGyroFile;
	std::mutex mGyroMutex;
	std::list< PendingGyro* > mPendingGyros;

	bool mConsumerRegistered;
};

#endif // RECORDERAVFORMAT_H
