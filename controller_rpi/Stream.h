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
#include <gammaengine/Socket.h>
#include <gammaengine/Renderer2D.h>
#include <gammaengine/Font.h>

#include "decode.h"
#include "DecodedImage.h"
#include "RendererHUD.h"

#include <wifibroadcast.h>

using namespace GE;

class Controller;

template<typename T> class HookThread : public Thread
{
public:
	HookThread( T* r, const std::function< bool( T* ) >& cb ) : Thread(), mT( r ), mCallback( cb ) {}
protected:
	virtual bool run() { return mCallback( mT ); }
private:
	T* mT;
	const std::function< bool( T* ) > mCallback;
};


class Stream : Thread
{
public:
	Stream( Controller* controller, Font* font, const std::string& addr, uint16_t port = 2021 );
	~Stream();

	static EGL_DISPMANX_WINDOW_T CreateNativeWindow( int layer );

protected:
	virtual bool run();
	bool DecodeThreadRun();
	bool SignalThreadRun();

private:
	Instance* mInstance;
	Window* mWindow;
	Renderer2D* mRenderer;
	Controller* mController;
	Font* mFont;
	Image* mVideoImage;
	Image* mBlankImage;
	uintptr_t mHUDColorId;
	uint64_t mHUDTicks;
	bool mBlinkingViews;
	Timer mHUDTimer;
	RendererHUD* mRendererHUD;

	rwifi_rx_t* mRx;
	Socket* mSocket;
	HookThread<Stream>* mDecodeThread;
	void* mDecodeInput;

	int mIwSocket;
	struct {
		int qual;
		int level;
		int noise;
		int channel;
	} mIwStats;
	HookThread<Stream>* mSignalThread;

	Timer mScreenTimer;
	Timer mFPSTimer;
	int mFPS;
	int mFrameCounter;
	int mBitrateCounter;

	EGL_DISPMANX_WINDOW_T mLayerDisplay;
	EGLImageKHR mEGLVideoImage;
	GLuint mVideoTexture;
	video_context* mDecodeContext;
};

#endif // STREAM_H
