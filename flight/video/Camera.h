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

#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include <string>
#include "VideoEncoder.h"

using namespace std;

class Camera
{
public:
	Camera();
	~Camera();

	typedef struct {
		uint8_t base;
		uint8_t radius;
		int8_t strength;
	} LensShaderColor;

	virtual void Start() = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
	virtual void StartRecording() = 0;
	virtual void StopRecording() = 0;
	virtual void TakePicture() = 0;

	virtual const uint32_t width() = 0;
	virtual const uint32_t height() = 0;
	virtual const uint32_t framerate() = 0;
	virtual const uint32_t brightness() = 0;
	virtual const int32_t contrast() = 0;
	virtual const int32_t saturation() = 0;
	virtual const int32_t ISO() = 0;
	virtual const uint32_t shutterSpeed() = 0;
	virtual const bool nightMode() = 0;
	virtual const string whiteBalance() = 0;
	virtual const string exposureMode() = 0;
	virtual const bool recording() = 0;
	virtual const string recordFilename() = 0;
	virtual void setBrightness( uint32_t value ) = 0;
	virtual void setContrast( int32_t value ) = 0;
	virtual void setSaturation( int32_t value ) = 0;
	virtual void setISO( int32_t value ) = 0;
	virtual void setShutterSpeed( uint32_t value ) = 0;
	virtual void setNightMode( bool night_mode ) = 0;
	virtual string switchWhiteBalance() = 0;
	virtual string lockWhiteBalance() = 0;
	virtual string switchExposureMode() = 0;
	virtual void setLensShader( const LensShaderColor& r, const LensShaderColor& g, const LensShaderColor& b ) = 0;
	virtual void getLensShader( LensShaderColor* r, LensShaderColor* g, LensShaderColor* b ) = 0;
	virtual uint32_t getLastPictureID() = 0;

	virtual uint32_t* getFileSnapshot( const string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp ) = 0;

protected:
	LUA_PROPERTY("preview_output") VideoEncoder* mLiveEncoder;
	LUA_PROPERTY("video_output") VideoEncoder* mVideoEncoder;
// 	LUA_PROPERTY("still_output") ImageEncoder* mStillEncoder;
};


#endif // CAMERA_H
