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

#include <GLES2/gl2.h>
#include <string>
#include "Vector.h"
#include "Matrix.h"

class Controller;

typedef struct VideoStats {
	int width;
	int height;
	int fps;
	char whitebalance[32];
} VideoStats;

typedef struct LinkStats {
	int qual;
	int level;
	int noise;
	int channel;
	char source;
} LinkStats;

typedef enum {
	Rate = 0,
	Stabilize = 1,
	ReturnToHome = 2,
	Follow = 3,
} DroneMode;

typedef struct DroneStats {
	// Status
	bool armed;
	DroneMode mode;
	uint32_t ping;
	// Attitude
	float thrust;
	float acceleration;
	Vector3f rpy;
	// Battery
	float batteryLevel;
	float batteryVoltage;
	uint32_t batteryTotalCurrent;
	// Other
	std::string username;
} DroneStats;

class RendererHUD
{
public:
	// render_region is top,bottom,left,right
	RendererHUD( int width, int height, float ratio, uint32_t fontsize, Vector4i render_region = Vector4i( 10, 10, 20, 20 ), bool barrel_correction = true );
	virtual ~RendererHUD();

	virtual void Compute() = 0;
	virtual void Render( DroneStats* dronestats, float localVoltage, VideoStats* videostats, LinkStats* iwstats ) = 0;

	void RenderQuadTexture( GLuint textureID, int x, int y, int width, int height, bool hmirror = false, bool vmirror = false );
	void RenderText( int x, int y, const std::string& text, uint32_t color, float size = 1.0f, bool hcenter = false );
	void RenderText( int x, int y, const std::string& text, const Vector4f& color, float size = 1.0f, bool hcenter = false );
	Vector2f VR_Distort( const Vector2f& coords );

	void PreRender();
	void setNightMode( bool m ) { mNightMode = m; }
	void setStereo( bool en ) { mStereo = en; if ( not mStereo ) { m3DStrength = 0.0f; } }
	void set3DStrength( float strength ) { m3DStrength = strength; }

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
		uint32_t mDistorID;
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
	Texture* LoadTexture( const uint8_t* data, uint32_t datalen );
	void LoadPNG( Texture* tex, std::istream& filename );

	void FontMeasureString( const std::string& str, int* width, int* height );
	int32_t CharacterWidth( Texture* tex, unsigned char c );
	int32_t CharacterHeight( Texture* tex, unsigned char c );
	int32_t CharacterYOffset( Texture* tex, unsigned char c );

	void DrawArrays( RendererHUD::Shader& shader, int mode, uint32_t ofs, uint32_t count, const Vector2f& offset = Vector2f() );

	uint32_t mDisplayWidth;
	uint32_t mDisplayHeight;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mBorderTop;
	uint32_t mBorderBottom;
	uint32_t mBorderLeft;
	uint32_t mBorderRight;
	bool mStereo;
	bool mNightMode;
	bool mBarrelCorrection;
	float m3DStrength;
	bool mBlinkingViews;
	float mHUDTick;

	Matrix* mMatrixProjection;
	uint32_t mQuadVBO;
	Shader mFlatShader;
	uint32_t mExposureID;
	uint32_t mGammaID;

	Texture* mFontTexture;
	uint32_t mFontSize;
	uint32_t mFontHeight;
	Shader mTextShader;
	uint32_t mTextVBO;
	int mTextAdv[256];

	std::string mWhiteBalance;
	float mWhiteBalanceTick;
};

#endif // RENDERERHUD_H
