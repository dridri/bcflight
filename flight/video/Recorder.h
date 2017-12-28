#ifndef RECORDER_H
#define RECORDER_H

#include <mutex>
#include <list>
#include <vector>
#include <stdint.h>
#include <Thread.h>

class Recorder : public Thread
{
public:
	typedef enum {
		TrackTypeVideo,
		TrackTypeAudio,
	} TrackType;

	Recorder();
	~Recorder();

	uint32_t AddVideoTrack( uint32_t width, uint32_t height, uint32_t average_fps, const std::string& extension );
	uint32_t AddAudioTrack( uint32_t channels, uint32_t sample_rate, const std::string& extension );
	void WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen );

protected:
	bool run();

	typedef struct {
		uint32_t id;
		TrackType type;
		char filename[256];
		FILE* file;
	} Track;

	typedef struct {
		Track* track;
		uint64_t record_time_us;
		uint8_t* buf;
		uint32_t buflen;
	} PendingSample;

	std::mutex mWriteMutex;
	std::vector< Track* > mTracks;
	std::list< PendingSample* > mPendingSamples;
	uint32_t mRecordId;
	FILE* mRecordFile;
	uint32_t mRecordSyncCounter;
};

#endif // RECORDER_H
