#ifndef RECORDER_H
#define RECORDER_H

#ifdef SYSTEM_NAME_Linux

#include <mutex>
#include <list>
#include <vector>
#include <stdint.h>
#include <Thread.h>

LUA_CLASS class Recorder : public Thread
{
public:
	typedef enum {
		TrackTypeVideo,
		TrackTypeAudio,
	} TrackType;

	LUA_EXPORT Recorder();
	~Recorder();

	void Start();
	void Stop();
	bool recording() const;

	uint32_t AddVideoTrack( uint32_t width, uint32_t height, uint32_t average_fps, const string& extension );
	uint32_t AddAudioTrack( uint32_t channels, uint32_t sample_rate, const string& extension );
	void WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen );

protected:
	bool run();

	typedef struct {
		uint32_t id;
		TrackType type;
		char filename[256];
		FILE* file;
		char extension[8];
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

#else // SYSTEM_NAME_Linux

class Recorder
{
public:
	void Start() {}
};

#endif // SYSTEM_NAME_Linux

#endif // RECORDER_H
