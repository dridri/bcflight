#ifndef RECORDERBASIC_H
#define RECORDERBASIC_H

#include "Recorder.h"
#include "Thread.h"

LUA_CLASS class RecorderBasic : public Recorder, protected Thread
{
public:
	LUA_EXPORT RecorderBasic();
	~RecorderBasic();

	virtual void Start();
	virtual void Stop();
	virtual bool recording();

	virtual uint32_t AddVideoTrack( uint32_t width, uint32_t height, uint32_t average_fps, const string& extension );
	virtual uint32_t AddAudioTrack( uint32_t channels, uint32_t sample_rate, const string& extension );
	virtual void WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen );
	virtual void WriteGyro( uint64_t record_time_us, const Vector3f& gyro, const Vector3f& accel = Vector3f() );

protected:
	bool run();

	typedef struct {
		uint32_t id;
		TrackType type;
		char filename[256];
		FILE* file;
		char extension[8];
		uint64_t recordStart;
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
	} PendingSample;

	LUA_PROPERTY("base_directory") string mBaseDirectory;
	mutex mWriteMutex;
	vector< Track* > mTracks;
	list< PendingSample* > mPendingSamples;
	uint32_t mRecordId;
	FILE* mRecordFile;
	uint32_t mRecordSyncCounter;
};

#endif // RECORDERBASIC_H
