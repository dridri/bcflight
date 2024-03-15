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
#include <vector>
#include <string>
#include <cstring>
#include "Vector.h"
#include "Matrix.h"

class Controller;

typedef struct VideoStats {
	VideoStats(int w = 0, int h = 0, int f = 0, uint32_t p = 0, const std::string& wb = "", const std::string& e = "")
		: width(w)
		, height(h)
		, fps(f)
		, photo_id(p)
		{
			strncpy( whitebalance, wb.c_str(), 32 );
			strncpy( exposure, e.c_str(), 32 );
		}
	int width;
	int height;
	int fps;
	uint32_t photo_id;
	char whitebalance[32];
	char exposure[32];
} VideoStats;

typedef struct LinkStats {
	LinkStats()
		: qual(0)
		, level(0)
		, noise(0)
		, channel(0)
		, source(0)
		{}
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
	DroneStats()
		: armed(false)
		, mode(DroneMode::Rate)
		, ping(0)
		, blackBoxId(0)
		, thrust(0)
		, acceleration(0)
		, rpy(Vector3f())
		, batteryLevel(0)
		, batteryVoltage(0)
		, batteryTotalCurrent(0)
		{}
	// Status
	bool armed;
	DroneMode mode;
	uint32_t ping;
	uint32_t blackBoxId;
	// Attitude
	float thrust;
	float acceleration;
	Vector3f rpy;
	Vector3f gpsLocation;
	float gpsSpeed;
	uint32_t gpsSatellitesSeen;
	uint32_t gpsSatellitesUsed;
	// Battery
	float batteryLevel;
	float batteryVoltage;
	uint32_t batteryTotalCurrent;
	// Other
	std::string username;
	std::vector< std::string > messages;
} DroneStats;

class RendererHUD
{
public:
	typedef enum {
		START = 0,
		CENTER = 1,
		END = 2,
	} TextAlignment;
	// render_region is top,bottom,left,right
	RendererHUD( int width, int height, float ratio, uint32_t fontsize, Vector4i render_region = Vector4i( 10, 10, 20, 20 ), bool barrel_correction = true );
	virtual ~RendererHUD();

	virtual void Compute() = 0;
	virtual void Render( DroneStats* dronestats, float localVoltage, VideoStats* videostats, LinkStats* iwstats ) = 0;

	void RenderQuadTexture( GLuint textureID, int x, int y, int width, int height, bool hmirror = false, bool vmirror = false, const Vector4f& color = { 1.0f, 1.0f, 1.0f, 1.0f } );
	void RenderText( int x, int y, const std::string& text, uint32_t color, float size = 1.0f, TextAlignment halign = TextAlignment::START, TextAlignment valign = TextAlignment::START );
	void RenderText( int x, int y, const std::string& text, const Vector4f& color, float size = 1.0f, TextAlignment halign = TextAlignment::START, TextAlignment valign = TextAlignment::START );
	void RenderImage( int x, int y, int width, int height, uintptr_t img );
	Vector2f VR_Distort( const Vector2f& coords );

	void PreRender();
	void setNightMode( bool m ) { mNightMode = m; }
	void setStereo( bool en ) { mStereo = en; if ( not mStereo ) { m3DStrength = 0.0f; } }
	void set3DStrength( float strength ) { m3DStrength = strength; }
	uintptr_t LoadImage( const std::string& path ) { return reinterpret_cast<uintptr_t>( LoadTexture(path) ); };
	
	bool nightMode() { return mNightMode; }

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
	std::string mExposureMode;
	float mWhiteBalanceTick;
	uint32_t mLastPhotoID;
	float mPhotoTick;
};

#endif // RENDERERHUD_H
