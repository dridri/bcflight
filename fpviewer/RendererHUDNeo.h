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

#ifndef RENDERERHUDNEO_H
#define RENDERERHUDNEO_H

#include <string>
#include "RendererHUD.h"
#include <Controller.h>

class RendererHUDNeo : public RendererHUD
{
public:
	RendererHUDNeo( int width, int height );
	~RendererHUDNeo();

	void Compute();
	void Render( Controller* controller, VideoStats* videostats, IwStats* iwstats );

	void RenderThrustAcceleration( float thrust, float acceleration );
	void RenderLink( float quality, float level );
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
	uint32_t mStaticLinesCountNoAttitude;
};

#endif // RENDERERHUDNEO_H
