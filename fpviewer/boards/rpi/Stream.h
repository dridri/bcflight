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

#include <Link.h>
#include <Thread.h>
#include "GLImage.h"
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

class Controller;

class Stream : public Thread
{
public:
	Stream( Link* link, uint32_t width, uint32_t height, float zoom, bool stretch, bool stereo, bool direct_render );
	~Stream();
	void setStereo( bool en );

	Link* link() const { return mLink; }
	int linkLevel() const { return mLink->RxLevel(); }
	int linkQuality() const { return mLink->RxQuality(); }
	int fps() const { return mFPS; }
	uint32_t width();
	uint32_t height();

	void Run() { while( run() ); }
	void Render( RendererHUD* renderer );

protected:
	virtual bool run();

private:
	bool mStereo;
	bool mDirectRender;
	uint32_t mScreenWidth;
	uint32_t mScreenHeight;
	uint32_t mWidth;
	uint32_t mHeight;
	float mZoom;
	bool mStretch;
	std::list< uint32_t > mHeadersReceived;

	Link* mLink;

	VID_INTF::VideoDecode* mDecoder;
	VID_INTF::VideoSplitter* mDecoderSplitter;
	VID_INTF::VideoRender* mDecoderRender;
	VID_INTF::EGLRender* mEGLRender;

	EGLImageKHR mEGLVideoImage;
	GLImage* mGLImage;

	int mFPS;
	int mFrameCounter;
	int mBitrateCounter;
	uint64_t mFPSTick;
};

#endif // STREAM_H
