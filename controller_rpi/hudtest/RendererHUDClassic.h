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
