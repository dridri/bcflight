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

#ifndef ALSAMIC_H
#define ALSAMIC_H

#if ( defined( SYSTEM_NAME_Linux ) )

#include <alsa/asoundlib.h>
extern "C" {
	#include <shine/layer3.h>
};
#include <Thread.h>
#include "Lua.h"
#include "Microphone.h"
#include "Recorder.h"


class Main;
class Link;

LUA_CLASS class AlsaMic : public Microphone
{
public:
	LUA_EXPORT AlsaMic();
	~AlsaMic();
	virtual void Setup();

protected:
	bool LiveThreadRun();
	int RecordWrite( char* data, int datalen );

	LUA_PROPERTY("device") std::string mDevice;
	LUA_PROPERTY("rate") uint32_t mRate;
	LUA_PROPERTY("channels") uint32_t mChannels;
	LUA_PROPERTY("link") Link* mLink;
	HookThread<AlsaMic>* mLiveThread;
	snd_pcm_t* mPCM;

	//Record
	LUA_PROPERTY("recorder") Recorder* mRecorder;
	uint32_t mRecorderTrackId;
	uint32_t mRecordSyncCounter;
	FILE* mRecordStream;
	shine_config_t mShineConfig;
	shine_t mShine;
};

#else

class AlsaMic {
public:
	static int flight_register( Main* main ) { return 0; }
};

#endif // SYSTEM_NAME_Linux

#endif // ALSAMIC_H
