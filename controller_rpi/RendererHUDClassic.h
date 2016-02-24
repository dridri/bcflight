#ifndef RENDERERHUDCLASSIC_H
#define RENDERERHUDCLASSIC_H

#include <string>
#include "RendererHUD.h"

class RendererHUDClassic : public RendererHUD
{
public:
	RendererHUDClassic( Instance* instance, Font* font );
	~RendererHUDClassic();

	void Compute();
	void Render( GE::Window* window, Controller* controller, VideoStats* videostats, IwStats* iwstats );

	void RenderThrust( float thrust );
	void RenderLink( float quality );
	void RenderBattery( float level );
	void RenderAttitude( const Vector3f& rpy );

protected:
	Vector3f mSmoothRPY;

	Shader mShader;
	Shader mColorShader;

	uint32_t mThrustVBO;
	uint32_t mLinkVBO;
	uint32_t mBatteryVBO;
	uint32_t mLineVBO;
};

#endif // RENDERERHUDCLASSIC_H
