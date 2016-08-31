/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef RASPICAM_H
#define RASPICAM_H

#include <fstream>
#include <functional>
#include <Thread.h>
#include <Config.h>
#include "Camera.h"

class Main;
class Link;
typedef struct video_context video_context;
typedef struct audio_context audio_context;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVStream AVStream;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodec AVCodec;

class Raspicam : public Camera
{
public:
	Raspicam( Config* config, const std::string& conf_obj );
	~Raspicam();

	virtual void StartRecording();
	virtual void StopRecording();

	virtual const uint32_t brightness() const;
	virtual const int32_t contrast() const;
	virtual const int32_t saturation() const;
	virtual void setBrightness( uint32_t value );
	virtual void setContrast( int32_t value );
	virtual void setSaturation( int32_t value );

protected:
	void SetupRecord();
	bool LiveThreadRun();
	bool RecordThreadRun();
	bool MixedThreadRun();

	int LiveSend( char* data, int datalen );
	int RecordWrite( char* data, int datalen, int64_t pts = 0, bool audio = false );

	Config* mConfig;
	std::string mConfigObject;
	Link* mLink;
	HookThread<Raspicam>* mLiveThread;
	HookThread<Raspicam>* mRecordThread;
	video_context* mVideoContext;
	audio_context* mAudioContext;
	bool mNeedNextEnc1ToBeFilled;
	bool mNeedNextEnc2ToBeFilled;
	bool mNeedNextAudioToBeFilled;
	bool mLiveSkipNextFrame;
	uint64_t mLiveFrameCounter;
	uint64_t mLiveTicks;
	uint64_t mRecordTicks;
	uint64_t mLedTick;
	bool mLedState;


	// Record
	bool mRecording;
	AVFormatContext* mRecordContext;
	AVStream* mAudioStream;
	AVStream* mVideoStream;
	uint64_t mRecordPTSBase;
	uint32_t mRecordFrameCounter;
// 	AVCodec* mAudioEncoder;
// 	AVCodecContext* mAudioEncoderContext;

// 	uint64_t mRecordTimestamp;
	char* mRecordFrameData;
	int mRecordFrameDataSize;
	int mRecordFrameSize;

	std::ofstream* mRecordStream; // TODO : use board-specific file instead
};

#endif // RASPICAM_H
