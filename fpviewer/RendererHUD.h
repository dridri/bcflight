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

#include <string>
#include "Vector.h"
#include "Matrix.h"

class Controller;

typedef struct VideoStats {
	int width;
	int height;
	int fps;
} VideoStats;

typedef struct IwStats {
	int qual;
	int level;
	int noise;
	int channel;
	char source;
} IwStats;

class RendererHUD
{
public:
	RendererHUD( int width, int height );
	virtual ~RendererHUD();

	virtual void Compute() = 0;
	virtual void Render( Controller* controller, float localVoltage, VideoStats* videostats, IwStats* iwstats ) = 0;

	void RenderQuadTexture( GLuint textureID, int x, int y, int width, int height, bool hmirror = false, bool vmirror = false );
	void RenderText( int x, int y, const std::string& text, uint32_t color, float size = 1.0f, bool hcenter = false );
	void RenderText( int x, int y, const std::string& text, const Vector4f& color, float size = 1.0f, bool hcenter = false );
	Vector2f VR_Distort( const Vector2f& coords );

	void PreRender( VideoStats* videostats );
	void setNightMode( bool m ) { mNightMode = m; }
	void setStereo( bool en ) { mStereo = en; }

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

	typedef struct {
		uint32_t width;
		uint32_t height;
		uint32_t* data;
		uint32_t glID;
	} Texture;

	int LoadVertexShader( RendererHUD::Shader* target, const void* data, size_t size );
	int LoadFragmentShader( RendererHUD::Shader* target, const void* data, size_t size );
	void createPipeline( Shader* target );
	Texture* LoadTexture( const std::string& filename );
	void LoadPNG( Texture* tex, const std::string& filename );

	void FontMeasureString( const std::string& str, int* width, int* height );
	int32_t CharacterWidth( Texture* tex, unsigned char c );
	int32_t CharacterHeight( Texture* tex, unsigned char c );
	int32_t CharacterYOffset( Texture* tex, unsigned char c );

	uint32_t mWidth;
	uint32_t mHeight;
	bool mStereo;
	bool mNightMode;
	float m3DStrength;
	bool mBlinkingViews;
	uint64_t mHUDTick;

	Matrix* mMatrixProjection;
	uint32_t mQuadVBO;
	Shader mFlatShader;

	Texture* mFontTexture;
	uint32_t mFontSize;
	Shader mTextShader;
	uint32_t mTextVBO;
	int mTextAdv[256];
};

#endif // RENDERERHUD_H
