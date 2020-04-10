#ifndef HUD_H
#define HUD_H

#if ( BUILD_HUD == 1 )

#include <Thread.h>
#include <GLContext.h>
#include <RendererHUD.h>

class HUD : public Thread
{
public:
	HUD( const string& type = "Neo" );
	~HUD();

	virtual bool run();

private:
	GLContext* mGLContext;
	RendererHUD* mRendererHUD;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mFrameRate;
	bool mNightMode;
	uint32_t mHUDFramerate;
	uint64_t mWaitTicks;
	bool mShowFrequency;
	float mAccelerationAccum;
};

#else // SYSTEM_NAME_Linux

class HUD
{
public:
};

#endif // ( BUILD_HUD == 1 )

#endif // HUD_H
