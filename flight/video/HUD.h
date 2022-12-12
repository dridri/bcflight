#ifndef HUD_H
#define HUD_H

#if ( BUILD_HUD == 1 )

#include <Thread.h>
#include <GLContext.h>
#include <RendererHUD.h>

LUA_CLASS class HUD : public Thread
{
public:
	LUA_EXPORT HUD();
	~HUD();

	virtual bool run();

private:
	GLContext* mGLContext;
	RendererHUD* mRendererHUD;
	uint32_t mWidth;
	uint32_t mHeight;
	LUA_PROPERTY("framerate") uint32_t mFrameRate;
	bool mNightMode;
	uint32_t mHUDFramerate;
	uint64_t mWaitTicks;
	LUA_PROPERTY("show_frequency") bool mShowFrequency;
	float mAccelerationAccum;
	LUA_PROPERTY("top") int32_t mFrameTop;
	LUA_PROPERTY("bottom") int32_t mFrameBottom;
	LUA_PROPERTY("left") int32_t mFrameLeft;
	LUA_PROPERTY("right") int32_t mFrameRight;
	LUA_PROPERTY("ratio") int32_t mRatio;
	LUA_PROPERTY("fontsize") int32_t mFontSize;
	LUA_PROPERTY("correction") bool mCorrection;
	LUA_PROPERTY("stereo") bool mStereo;
	LUA_PROPERTY("stereo_strength") float mStereoStrength;

	DroneStats mDroneStats;
	LinkStats mLinkStats;
	VideoStats mVideoStats;
};

#else // SYSTEM_NAME_Linux

class HUD
{
public:
};

#endif // ( BUILD_HUD == 1 )

#endif // HUD_H
