#ifndef HUD_H
#define HUD_H

#include <Thread.h>
#include <GLContext.h>
#include <RendererHUD.h>

class HUD : public Thread
{
public:
	HUD( const std::string& type = "Neo" );
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
};

#endif // HUD_H
