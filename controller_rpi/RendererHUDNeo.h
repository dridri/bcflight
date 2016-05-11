#ifndef RENDERERHUDNEO_H
#define RENDERERHUDNEO_H

#include <string>
#include "RendererHUD.h"
#include <Controller.h>

class RendererHUDNeo : public RendererHUD
{
public:
	RendererHUDNeo( Instance* instance, Font* font );
	~RendererHUDNeo();

	void Compute();
	void Render( GE::Window* window, Controller* controller, VideoStats* videostats, IwStats* iwstats );

	void RenderThrustAcceleration( float thrust, float acceleration );
	void RenderLink( float quality );
	void RenderBattery( float level );
	void RenderAttitude( const vec3& rpy );

protected:
	Vector3f mSmoothRPY;

	Shader mShader;
	Shader mColorShader;

	uint32_t mThrustVBO;
	uint32_t mAccelerationVBO;
	uint32_t mLinkVBO;
	uint32_t mBatteryVBO;
	uint32_t mLineVBO;
	uint32_t mCircleVBO;
	uint32_t mStaticLinesVBO;

	uint32_t mStaticLinesCount;
};

#endif // RENDERERHUDNEO_H
