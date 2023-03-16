#ifndef HUD_H
#define HUD_H

#if ( BUILD_HUD == 1 )

#include <stdint.h>
#include <Thread.h>
#include <RendererHUD.h>
#include <map>

class GLContext;

LUA_CLASS class HUD : public Thread
{
public:
	LUA_EXPORT HUD();
	~HUD();

	LUA_EXPORT uintptr_t LoadImage( const std::string& path );
	LUA_EXPORT void ShowImage( int32_t x, int32_t y, uint32_t w, uint32_t h, const uintptr_t img );
	LUA_EXPORT void HideImage( const uintptr_t img );

	virtual bool run();

private:
	GLContext* mGLContext;
	RendererHUD* mRendererHUD;
	LUA_PROPERTY("framerate") uint32_t mFrameRate;
	LUA_PROPERTY("night") bool mNightMode;
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

	typedef struct {
		uint64_t img;
		int32_t x;
		int32_t y;
		uint32_t width;
		uint32_t height;
	} Image;

	DroneStats mDroneStats;
	LinkStats mLinkStats;
	VideoStats mVideoStats;
	std::map< uintptr_t, Image > mImages;
	std::mutex mImagesMutex;
};

#else // SYSTEM_NAME_Linux

class HUD
{
public:
};

#endif // ( BUILD_HUD == 1 )

#endif // HUD_H
