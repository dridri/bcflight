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

#include <vc_dispmanx_types.h>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <gammaengine/Instance.h>
#include <gammaengine/Window.h>
#include <gammaengine/Thread.h>
#include <gammaengine/Timer.h>
#include <gammaengine/Renderer2D.h>
#include <gammaengine/Font.h>

#include <Link.h>
#include <Thread.h>
#include "decode.h"
#include "DecodedImage.h"
#include "RendererHUD.h"

// #include <wifibroadcast.h>

using namespace GE;

class Controller;

class Stream : GE::Thread
{
public:
	Stream( Controller* controller, Font* font, Link* link, uint32_t width, uint32_t height, bool stereo );
	~Stream();
	void setRenderHUD( bool en ) { mRenderHUD = en; }
	void setStereo( bool en );

	Link* link() const { return mLink; }
	int linkLevel() const { return mIwStats.level; }
	int linkQuality() const { return mIwStats.qual; }

	EGL_DISPMANX_WINDOW_T CreateNativeWindow( int layer );

protected:
	virtual bool run();
	bool DecodeThreadRun();
	bool SignalThreadRun();

private:
	Instance* mInstance;
	Window* mWindow;
	Controller* mController;
	Font* mFont;
	Timer mSecondTimer;
	RendererHUD* mRendererHUD;
	bool mRenderHUD;
	bool mStereo;
	uint32_t mWidth;
	uint32_t mHeight;

	Link* mLink;
	HookThread<Stream>* mDecodeThread;
	void* mDecodeInput;
	uint32_t mDecodeLen;

	int mIwSocket;
	IwStats mIwStats;
	HookThread<Stream>* mSignalThread;

	Timer mFPSTimer;
	int mFPS;
	int mFrameCounter;
	int mBitrateCounter;

	EGL_DISPMANX_WINDOW_T mLayerDisplay;
	EGLImageKHR mEGLVideoImage;
	GLuint mVideoTexture;
	video_context* mDecodeContext;

	DISPMANX_DISPLAY_HANDLE_T mDisplay;
	struct {
		DISPMANX_RESOURCE_HANDLE_T resource;
		VC_RECT_T rect;
		void *image;
		uint32_t vc_image_ptr;
	} mScreenshot;
};

#endif // STREAM_H
