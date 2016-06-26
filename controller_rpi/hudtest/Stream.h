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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <gammaengine/Instance.h>
#include <gammaengine/Window.h>
#include <gammaengine/Thread.h>
#include <gammaengine/Timer.h>
#include <gammaengine/Socket.h>
#include <gammaengine/Renderer2D.h>
#include <gammaengine/Font.h>

#include "RendererHUD.h"

using namespace GE;

class Controller;

class Stream : Thread
{
public:
	Stream( Controller* controller, Font* font, const std::string& addr, uint16_t port = 2021 );
	~Stream();
	bool Run() { return run(); }

protected:
	virtual bool run();
	bool DecodeThreadRun();
	bool SignalThreadRun();

private:
	Instance* mInstance;
	GE::Window* mWindow;
	Renderer2D* mRenderer;
	Controller* mController;
	Font* mFont;
	Image* mVideoImage;
	Image* mBlankImage;
	uintptr_t mHUDColorId;
	uint64_t mHUDTicks;
	RendererHUD* mRendererHUD;

	void* mDecodeInput;

	int mIwSocket;
	IwStats mIwStats;

	Timer mScreenTimer;
	Timer mFPSTimer;
	int mFPS;
	int mFrameCounter;
	int mBitrateCounter;
};

#endif // STREAM_H
