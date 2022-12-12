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

#include <stdio.h>
#include <fstream>
#include <functional>
#include <Thread.h>
#include <Config.h>
#include "Camera.h"

#define CAM_USE_MMAL 0

#if CAM_USE_MMAL == 1
#define CAM_INTF MMAL
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/Camera.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/VideoSplitter.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/VideoEncode.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/VideoDecode.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/VideoRender.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/EGLRender.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/ImageEncode.h"
#else
#define CAM_INTF IL
#include "../../external/OpenMaxIL++/include/Component.h"
#include "../../external/OpenMaxIL++/include/Camera.h"
#include "../../external/OpenMaxIL++/include/VideoSplitter.h"
#include "../../external/OpenMaxIL++/include/VideoEncode.h"
#include "../../external/OpenMaxIL++/include/VideoDecode.h"
#include "../../external/OpenMaxIL++/include/VideoRender.h"
#include "../../external/OpenMaxIL++/include/EGLRender.h"
#include "../../external/OpenMaxIL++/include/ImageEncode.h"
#endif

class Main;
class Link;

LUA_CLASS class Raspicam : public Camera
{
public:
	LUA_EXPORT Raspicam();
	~Raspicam();

	virtual void Start();
	virtual void Pause();
	virtual void Resume();
	virtual void StartRecording();
	virtual void StopRecording();
	virtual void TakePicture();

	virtual const uint32_t framerate();
	virtual const uint32_t brightness();
	virtual const int32_t contrast();
	virtual const int32_t saturation();
	virtual const int32_t ISO();
	virtual const uint32_t shutterSpeed();
	virtual const bool nightMode();
	virtual const string whiteBalance();
	virtual const string exposureMode();
	virtual const bool recording();
	virtual const string recordFilename();
	virtual void setBrightness( uint32_t value );
	virtual void setContrast( int32_t value );
	virtual void setSaturation( int32_t value );
	virtual void setISO( int32_t value );
	virtual void setShutterSpeed( uint32_t value );
	virtual void setNightMode( bool night_mode );
	virtual string switchWhiteBalance();
	virtual string lockWhiteBalance();
	virtual string switchExposureMode();
	virtual void setLensShader( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B );
	virtual void getLensShader( LensShaderColor* r, LensShaderColor* g, LensShaderColor* b );
	virtual uint32_t getLastPictureID();

	virtual uint32_t* getFileSnapshot( const string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp );

	LUA_CLASS class ModelSettings {
	public:
		LUA_EXPORT ModelSettings();
		LUA_PROPERTY("sensor_mode") int32_t sensorMode;
		LUA_PROPERTY("width") int32_t width;
		LUA_PROPERTY("height") int32_t height;
		LUA_PROPERTY("fps") int32_t fps;
		LUA_PROPERTY("exposure") int32_t exposure;
		LUA_PROPERTY("iso") int32_t iso;
		LUA_PROPERTY("shutter_speed") int32_t shutterSpeed;
		LUA_PROPERTY("sharpness") int32_t sharpness;
		LUA_PROPERTY("brightness") int32_t brightness;
		LUA_PROPERTY("contrast") int32_t contrast;
		LUA_PROPERTY("saturation") int32_t saturation;
		LUA_PROPERTY("vflip") bool vflip;
		LUA_PROPERTY("hflip") bool hflip;
		LUA_PROPERTY("whiteBalance") std::string whiteBalance;
		LUA_PROPERTY("stabilisation") bool stabilisation;
		LUA_PROPERTY("night_fps") int32_t night_fps;
		LUA_PROPERTY("night_iso") int32_t night_iso;
		LUA_PROPERTY("night_brightness") int32_t night_brightness;
		LUA_PROPERTY("night_contrast") int32_t night_contrast;
		LUA_PROPERTY("night_saturation") int32_t night_saturation;
	};

protected:
	bool LiveThreadRun();
	bool RecordThreadRun();
	bool TakePictureThreadRun();

	int LiveSend( char* data, int datalen );
	int RecordWrite( char* data, int datalen, int64_t pts = 0, bool audio = false );
	void setLensShader_internal( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B );

	CAM_INTF::Camera* mHandle;

	LUA_PROPERTY("link") Link* mLink;
	LUA_PROPERTY("direct_mode") bool mDirectMode;
	string mName;
	LUA_PROPERTY("v1") Raspicam::ModelSettings mSettingsV1;
	LUA_PROPERTY("v2") Raspicam::ModelSettings mSettingsV2;
	LUA_PROPERTY("hq") Raspicam::ModelSettings mSettingsHQ;
	Raspicam::ModelSettings* mSettings;
	uint32_t mWidth;
	uint32_t mHeight;
	int32_t mISO;
	uint32_t mShutterSpeed;
	CAM_INTF::VideoEncode* mEncoder;
	// CAM_INTF::VideoRender* mRender;
	CAM_INTF::EGLRender* mRender;
	HookThread<Raspicam>* mLiveThread;
	uint64_t mLiveFrameCounter;
	uint64_t mLiveTicks;
	uint64_t mRecordTicks;
	uint64_t mLedTick;
	uint64_t mHeadersTick;
	bool mLedState;
	bool mNightMode;
	bool mPaused;
	CAM_INTF::Camera::WhiteBalControl mWhiteBalance;
	CAM_INTF::Camera::ExposureControl mExposureMode;
	string mWhiteBalanceLock;
	uint8_t* mLiveBuffer;
	LUA_PROPERTY("disable_lens_shading") bool mDisableLensShading;
	LensShaderColor mLensShaderR;
	LensShaderColor mLensShaderG;
	LensShaderColor mLensShaderB;

	// Record
	bool mBetterRecording;
	bool mRecording;
	string mRecordFilename;
	char* mRecordFrameData;
	int mRecordFrameDataSize;
	int mRecordFrameSize;
	LUA_PROPERTY("kbps") uint32_t mRecordBitRateKbps;
	uint32_t mRecordSyncCounter;
	CAM_INTF::VideoSplitter* mSplitter;
	CAM_INTF::VideoEncode* mEncoderRecord;
	HookThread<Raspicam>* mRecordThread;

	// Still image
	bool mTakingPicture;
	HookThread<Raspicam>* mTakePictureThread;
	CAM_INTF::ImageEncode* mImageEncoder;
	mutex mTakePictureMutex;
	condition_variable mTakePictureCond;
	uint32_t mLastPictureID;

	FILE* mRecordStream; // TODO : use board-specific file instead
	mutex mRecordStreamMutex;
	list< pair< uint8_t*, uint32_t > > mRecordStreamQueue;
	uint32_t mRecorderTrackId;

	static void DebugOutput( int level, const string fmt, ... );
};

#endif // RASPICAM_H
