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

#ifndef STREAM_H
#define STREAM_H

//TEST
#define VID_INTF_ID 0 // IL=0, MMAL=1
// #define VID_INTF MMAL
#define VID_INTF IL
//ENDTEST

#include <vc_dispmanx_types.h>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <Link.h>
#include <Thread.h>
#if ( VID_INTF_ID == 0 )
#include "../../external/OpenMaxIL++/include/VideoDecode.h"
#include "../../external/OpenMaxIL++/include/VideoSplitter.h"
#include "../../external/OpenMaxIL++/include/VideoRender.h"
#include "../../external/OpenMaxIL++/include/EGLRender.h"
#elif ( VID_INTF_ID == 1 )
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/VideoDecode.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/VideoSplitter.h"
#include "../../external/OpenMaxIL++/MMAL++/include/MMAL++/VideoRender.h"
#endif
#include "RendererHUD.h"
#include "DecodedImage.h"

class Controller;

class Stream : Thread
{
public:
	Stream( Controller* controller, Link* link, uint32_t width, uint32_t height, bool stereo );
	~Stream();
	void setRenderHUD( bool en ) { mRenderHUD = en; }
	void setStereo( bool en );

	Link* link() const { return mLink; }
	int linkLevel() const { return mIwStats.level; }
	int linkQuality() const { return mIwStats.qual; }

	EGL_DISPMANX_WINDOW_T CreateNativeWindow( int layer );
	void Run() { while( run() ); }

protected:
	virtual bool run();
	bool DecodeThreadRun();
	bool SignalThreadRun();

private:
	Controller* mController;
	RendererHUD* mRendererHUD;
	bool mRenderHUD;
	bool mStereo;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mEGLWidth;
	uint32_t mEGLHeight;

	Link* mLink;
	typedef struct Packet {
		uint8_t data[32768];
		uint32_t size;
	} Packet;
	std::list< Packet* > mPacketsQueue;

	HookThread<Stream>* mDecodeThread;
	void* mDecodeInput;
	uint32_t mDecodeLen;
	VID_INTF::VideoDecode* mDecoder;
	VID_INTF::VideoSplitter* mDecoderSplitter;
	VID_INTF::VideoRender* mDecoderRender1;
	VID_INTF::VideoRender* mDecoderRender2;
	VID_INTF::EGLRender* mEGLRender;

	int mIwSocket;
	IwStats mIwStats;
	HookThread<Stream>* mSignalThread;

	int mFPS;
	int mFrameCounter;
	int mBitrateCounter;
	uint64_t mFPSTick;

	EGL_DISPMANX_WINDOW_T mLayerDisplay;
	EGLImageKHR mEGLVideoImage;
	DecodedImage* mVideoImage;

	EGLDisplay mEGLDisplay;
	EGLConfig mEGLConfig;
	EGLContext mEGLContext;
	EGLSurface mEGLSurface;
	DISPMANX_DISPLAY_HANDLE_T mDisplay;
	struct {
		DISPMANX_RESOURCE_HANDLE_T resource;
		VC_RECT_T rect;
		void *image;
		uint32_t vc_image_ptr;
	} mScreenshot;
};

#endif // STREAM_H
