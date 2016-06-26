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

#ifndef RENDERERHUD_H
#define RENDERERHUD_H

#include <gammaengine/Instance.h>
#include <gammaengine/Window.h>
#include <gammaengine/Timer.h>
#include <gammaengine/Font.h>
#include <gammaengine/Image.h>
#include <gammaengine/Matrix.h>
#include <string>

using namespace GE;

class Controller;

typedef struct VideoStats {
	int fps;
} VideoStats;


typedef struct IwStats {
	int qual;
	int level;
	int noise;
	int channel;
} IwStats;


class RendererHUD
{
public:
	RendererHUD( Instance* instance, Font* font );
	virtual ~RendererHUD();

	virtual void Compute() = 0;
	virtual void Render( GE::Window* window, Controller* controller, VideoStats* videostats, IwStats* iwstats ) = 0;

	void RenderText( int x, int y, const std::string& text, uint32_t color, float size = 1.0f, bool hcenter = false );
	void RenderText( int x, int y, const std::string& text, const Vector4f& color, float size = 1.0f, bool hcenter = false );
	Vector2f VR_Distort( const Vector2f& coords );

	void PreRender( VideoStats* videostats );
	void setNightMode( bool m ) { mNightMode = m; }

protected:
	typedef struct {
		float u, v;
		float x, y;
	} FastVertex;

	typedef struct {
		float u, v;
		uint32_t color;
		float x, y;
	} FastVertexColor;

	typedef struct {
		uint32_t mShader;
		uint32_t mVertexShader;
		uint32_t mFragmentShader;
		uint32_t mVertexTexcoordID;
		uint32_t mVertexColorID;
		uint32_t mVertexPositionID;
		uint32_t mMatrixProjectionID;
		uint32_t mOffsetID;
		uint32_t mScaleID;
		uint32_t mColorID;
	} Shader;

	int LoadVertexShader( RendererHUD::Shader* target, const void* data, size_t size );
	int LoadFragmentShader( RendererHUD::Shader* target, const void* data, size_t size );
	uint8_t* loadShader( const std::string& filename, size_t* sz = 0 );
	void createPipeline( Shader* target );

	Instance* mInstance;
	Font* mFont;
	bool mNightMode;
	float m3DStrength;
	bool mBlinkingViews;
	Timer mHUDTimer;

	Matrix* mMatrixProjection;

	Shader mTextShader;
	uint32_t mTextVBO;
	int mTextAdv[256];
};

#endif // RENDERERHUD_H
