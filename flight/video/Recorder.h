#ifndef RECORDER_H
#define RECORDER_H

#ifdef SYSTEM_NAME_Linux

#include <mutex>
#include <set>
#include <list>
#include <vector>
#include <stdint.h>
#include "Lua.h"
#include "Vector.h"

class Recorder
{
public:
	typedef enum {
		TrackTypeVideo,
		TrackTypeAudio,
	} TrackType;

	Recorder();
	~Recorder();

	LUA_EXPORT virtual void Start() = 0;
	LUA_EXPORT virtual void Stop() = 0;
	LUA_EXPORT virtual bool recording() = 0;

	virtual uint32_t AddVideoTrack( uint32_t width, uint32_t height, uint32_t average_fps, const string& extension ) = 0;
	virtual uint32_t AddAudioTrack( uint32_t channels, uint32_t sample_rate, const string& extension ) = 0;
	virtual void WriteSample( uint32_t track_id, uint64_t record_time_us, void* buf, uint32_t buflen ) = 0;

	virtual void WriteGyro( uint64_t record_time_us, const Vector3f& gyro, const Vector3f& accel = Vector3f() ) = 0;

	static const std::set<Recorder*>& recorders();

protected:
	static std::set<Recorder*> sRecorders;
};

#else // SYSTEM_NAME_Linux

class Recorder
{
public:
	void Start() {}
};

#endif // SYSTEM_NAME_Linux

#endif // RECORDER_H
