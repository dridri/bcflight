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

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <string.h>
#include <png.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <Thread.h>
#include "RendererHUD.h"
#include "Font.h"
#include "font32.h"

static const char hud_flat_vertices_shader[] =
R"(	#version 100
	precision mediump float;

	#define in attribute
	#define out varying
	#define ge_Position gl_Position
	in vec2 ge_VertexTexcoord;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2 offset;
	uniform float scale;
	out vec2 ge_TextureCoord;

	void main()
	{
		ge_TextureCoord = ge_VertexTexcoord.xy;
		vec2 pos = offset + ge_VertexPosition.xy * scale;
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
	})"
;

static const char hud_flat_fragment_shader[] =
R"(	#version 100
	precision mediump float;

	#define in varying
	#define ge_FragColor gl_FragColor
	uniform sampler2D ge_Texture0;
	uniform float exposure_value;
	uniform float gamma_compensation;

	uniform vec4 color;
	in vec2 ge_TextureCoord;

	void main()
	{
		ge_FragColor = color * texture2D( ge_Texture0, ge_TextureCoord.xy );
		return;

// 		uniform float kernel[9] = float[]( 0, -1, 0, -1, 5, -1, 0, -1, 0 );
		ge_FragColor = vec4(0.0);

		ge_FragColor += 5.0 * texture2D( ge_Texture0, ge_TextureCoord.xy );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2(-0.000976, 0.0 ) );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2( 0.001736, 0.0 ) );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2( 0.0,-0.000976 ) );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2( 0.0, 0.001736 ) );

// 		ge_FragColor += texture2D( ge_Texture0, ge_TextureCoord.xy );

		ge_FragColor *= color;
// 		ge_FragColor.a = 1.0;

		ge_FragColor.rgb = vec3(1.0) - exp( -ge_FragColor.rgb * vec3(exposure_value) );
		ge_FragColor.rgb = pow( ge_FragColor.rgb, vec3( 1.0 / gamma_compensation ) );
	})"
;

static const char hud_rrect_vertices_shader[] =
R"(	#version 100
	precision mediump float;

	#define in attribute
	#define out varying
	#define ge_Position gl_Position
	in vec2 ge_VertexTexcoord;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2 offset;
	out vec2 ge_TextureCoord;

	void main()
	{
		ge_TextureCoord = ge_VertexTexcoord.xy;
		vec2 pos = offset + ge_VertexPosition.xy;
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
	})"
;

static const char hud_rrect_fragment_shader[] =
R"(	#version 100
	precision mediump float;

	#define in varying
	#define ge_FragColor gl_FragColor
	in vec2 ge_TextureCoord;

	uniform vec2  rect_size;
	uniform float radius;
	uniform float stroke_width;
	uniform vec4  color;
	uniform vec4  stroke_color;

	float roundedBoxSDF( vec2 p, vec2 halfSize, float r ) {
		vec2 d = abs(p) - halfSize + r;
		return length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0) - r;
	}

	void main()
	{
		vec2 p = (ge_TextureCoord - vec2(0.5)) * rect_size;
		float d = roundedBoxSDF(p, rect_size * 0.5, radius);
		// outer edge AA (bord extérieur de la forme)
		float outer_aa = smoothstep(1.0, -1.0, d);
		if ( stroke_width > 0.0 && d > -stroke_width ) {
			// inner edge AA (bord intérieur du stroke = frontière fill/stroke)
			float inner_aa = smoothstep(-1.0, 1.0, d + stroke_width);
			float stroke_alpha = outer_aa * inner_aa;
			ge_FragColor = vec4(stroke_color.rgb, stroke_color.a * stroke_alpha);
		} else {
			ge_FragColor = vec4(color.rgb, color.a * outer_aa);
		}
	})"
;

static const char hud_gauge_vertices_shader[] =
R"(	#version 100
	precision mediump float;

	#define in attribute
	#define out varying
	#define ge_Position gl_Position

	in vec2 ge_VertexTexcoord;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2  gauge_center;
	uniform float gauge_radius;
	uniform float gauge_inner_ratio;
	out float v_band;

	void main()
	{
		float r = mix(gauge_radius * gauge_inner_ratio, gauge_radius, ge_VertexTexcoord.x);
		v_band = ge_VertexTexcoord.y;
		vec2 pos = gauge_center + r * ge_VertexPosition.xy;
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
	})"
;

static const char hud_gauge_fragment_shader[] =
R"(	#version 100
	precision mediump float;

	#define in varying
	#define ge_FragColor gl_FragColor

	uniform vec4 color;
	uniform vec4 glow_color;
	in float v_band;

	void main()
	{
		ge_FragColor = vec4(0.0);
		if ( v_band >= 1.0 ) {
			ge_FragColor = vec4(color.rgb, color.a * v_band );
		} else {
			ge_FragColor += vec4(glow_color.rgb, glow_color.a * v_band );
		}
	})"
;


static const char hud_text_vertices_shader[] =
R"(	#version 100
	precision mediump float;

	#define in attribute
	#define out varying
	#define ge_Position gl_Position
	in vec2 ge_VertexTexcoord;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2 offset;
	uniform float scale;
	uniform float distort;
	out vec2 ge_TextureCoord;

	void main()
	{
		ge_TextureCoord = ge_VertexTexcoord.xy;
		vec2 pos = offset + ge_VertexPosition.xy * scale;
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
	})"
;

static const char hud_text_fragment_shader[] =
R"(	#version 100
	precision mediump float;

	#define in varying
	#define ge_FragColor gl_FragColor
	uniform sampler2D ge_Texture0;

	uniform vec4 color;
	uniform vec4 outline_color;
	uniform vec4 shadow_color;
	in vec2 ge_TextureCoord;

	void main()
	{
		vec4 t = texture2D( ge_Texture0, ge_TextureCoord.xy );
		float a_fill     = clamp( t.r * color.a, 0.0, 1.0 );
		float a_outline  = clamp( (t.g - t.r) * outline_color.a, 0.0, 1.0 );
		float a_shadow   = clamp( t.b * shadow_color.a, 0.0, 1.0 );
		vec3 rgb_shadow  = shadow_color.rgb * step( 0.0001, a_shadow );
		vec3 rgb_outline = outline_color.rgb * step( 0.0001, a_outline );
		vec3 rgb_fill    = color.rgb * step( 0.0001, a_fill );

		vec3  rgb = vec3(0.0);
		float a   = 0.0;

		if ( a_outline == 0.0 && a_shadow == 0.0 ) {
			rgb = rgb_fill;
			a   = a_fill;
		} else if ( a_outline == 0.0 && a_fill == 0.0 ) {
			rgb = rgb_shadow;
			a   = a_shadow;
		} else if ( a_outline == 0.0 ) {
			rgb = rgb_shadow;
			a   = a_shadow;
			rgb = mix( rgb, rgb_fill, a_fill );
			a   = a_fill + a * ( 1.0 - a_fill );
		} else {
			rgb = mix( rgb, rgb_outline, a_outline );
			a   = a_outline + a * ( 1.0 - a_outline );
			rgb = mix( rgb, rgb_fill, a_fill );
			a   = a_fill + a * ( 1.0 - a_fill );
		}

		ge_FragColor = vec4( rgb, a );
	})"
;


Vector2f RendererHUD::VR_Distort( const Vector2f& coords )
{
	Vector2f ret = coords;
/*
	Vector2f ofs = Vector2f( 1280.0 / 4.0, 720.0 / 2.0 );
	if ( coords.x >= 1280.0 / 2.0 ) {
		ofs.x = 3.0 * 1280.0 / 4.0;
	}
	Vector2f offset = coords - ofs;
	float r2 = offset.x * offset.x + offset.y * offset.y;
	float r = std::sqrt( r2 );
	float k1 = 1.95304;
	k1 = k1 / ( ( 1280.0 / 4.0 ) * ( 1280.0 / 4.0 ) * 16.0 );

	ret = ofs + ( mBarrelCorrection ? ( 1.0f - k1 * r * r ) : 1.0f ) * offset;
*/
	return ret;
}


void RendererHUD::DrawArrays( RendererHUD::Shader& shader, int mode, uint32_t ofs, uint32_t count, const Vector2f& offset )
{
	glUniform2f( shader.mOffsetID, offset.x, offset.y );
	glDrawArrays( mode, ofs, count );
}


RendererHUD::RendererHUD( int width, int height, float ratio, uint32_t fontsize, Vector4i render_region, bool barrel_correction )
	: mDisplayWidth( width )
	, mDisplayHeight( height )
	, mWidth( 1280 )
	, mHeight( 720 )
	, mBorderTop( render_region.x )
	, mBorderBottom( mHeight - render_region.y )
	, mBorderLeft( render_region.z )
	, mBorderRight( mWidth - render_region.w )
	, mStereo( false )
	, mNightMode( false )
	, mBarrelCorrection( barrel_correction )
	, m3DStrength( 0.004f )
	, mBlinkingViews( false )
	, mHUDTick( 0 )
	, mMatrixProjection( new Matrix() )
	, mQuadVBO( 0 )
	, mFlatShader( { 0 } )
	, mDefaultFont( nullptr )
	, mFontSize( fontsize )
	, mFontHeight( fontsize * 0.65f )
	, mTextShader{ 0 }
	, mWhiteBalance( "auto" )
	, mExposureMode( "auto" )
	, mWhiteBalanceTick( 0.0f )
	, mLastPhotoID( 0 )
	, mPhotoTick( 0.0f )
{
	mWidth *= ratio;
	mBorderLeft *= ratio;
	mBorderRight *= ratio;
	mMatrixProjection->Orthogonal( 0.0, 1280 * ratio, 720, 0.0, -2049.0, 2049.0 );

	glDisable( GL_DEPTH_TEST );
	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glLineWidth( 2.0f );

	LoadVertexShader( &mFlatShader, hud_flat_vertices_shader, sizeof(hud_flat_vertices_shader) + 1 );
	LoadFragmentShader( &mFlatShader, hud_flat_fragment_shader, sizeof(hud_flat_fragment_shader) + 1 );
	createPipeline( &mFlatShader );
	glUseProgram( mFlatShader.mShader );
	glEnableVertexAttribArray( mFlatShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mFlatShader.mVertexPositionID );
	mExposureID = glGetUniformLocation( mFlatShader.mShader, "exposure_value" );
	mGammaID = glGetUniformLocation( mFlatShader.mShader, "gamma_compensation" );
	std::cout << "mExposureID : " << mExposureID << "\n";
	std::cout << "mGammaID : " << mGammaID << "\n";
	std::cout << "mOffsetID : " << mFlatShader.mOffsetID << "\n";
	glUseProgram( 0 );

	LoadVertexShader( &mTextShader, hud_text_vertices_shader, sizeof(hud_text_vertices_shader) + 1 );
	LoadFragmentShader( &mTextShader, hud_text_fragment_shader, sizeof(hud_text_fragment_shader) + 1 );
	createPipeline( &mTextShader );
	glUseProgram( mTextShader.mShader );
	glEnableVertexAttribArray( mTextShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mTextShader.mVertexPositionID );
	mTextOutlineColorID = glGetUniformLocation( mTextShader.mShader, "outline_color" );
	mTextShadowColorID = glGetUniformLocation( mTextShader.mShader, "shadow_color" );
	glUseProgram( 0 );

	LoadVertexShader( &mRRectShader, hud_rrect_vertices_shader, sizeof(hud_rrect_vertices_shader) + 1 );
	LoadFragmentShader( &mRRectShader, hud_rrect_fragment_shader, sizeof(hud_rrect_fragment_shader) + 1 );
	createPipeline( &mRRectShader );
	glUseProgram( mRRectShader.mShader );
	glEnableVertexAttribArray( mRRectShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mRRectShader.mVertexPositionID );
	mRRectSizeID    = glGetUniformLocation( mRRectShader.mShader, "rect_size" );
	mRRectRadiusID  = glGetUniformLocation( mRRectShader.mShader, "radius" );
	mRRectStrokeWID = glGetUniformLocation( mRRectShader.mShader, "stroke_width" );
	mRRectStrokeCID = glGetUniformLocation( mRRectShader.mShader, "stroke_color" );
	glUseProgram( 0 );

	LoadVertexShader( &mGaugeShader, hud_gauge_vertices_shader, sizeof(hud_gauge_vertices_shader) + 1 );
	LoadFragmentShader( &mGaugeShader, hud_gauge_fragment_shader, sizeof(hud_gauge_fragment_shader) + 1 );
	createPipeline( &mGaugeShader );
	glUseProgram( mGaugeShader.mShader );
	glEnableVertexAttribArray( mGaugeShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mGaugeShader.mVertexPositionID );
	mGaugeShader.mGlowColorID = glGetUniformLocation( mGaugeShader.mShader, "glow_color" );
	mGaugeCenterID     = glGetUniformLocation( mGaugeShader.mShader, "gauge_center" );
	mGaugeRadiusID     = glGetUniformLocation( mGaugeShader.mShader, "gauge_radius" );
	mGaugeInnerRatioID = glGetUniformLocation( mGaugeShader.mShader, "gauge_inner_ratio" );
	glUseProgram( 0 );

	{
		const int N = 128;
		// 3 bands of N*4 vertices each: glow_inner, solid, glow_outer
		FastVertex gaugeBuffer[N * 4 * 3];
		for ( int i = 0; i < N; i++ ) {
			float a0 = (float)i       / N * M_PI * 2.0f;
			float a1 = (float)(i + 1) / N * M_PI * 2.0f;
			float dx0 = -sinf(a0), dy0 = cosf(a0);
			float dx1 = -sinf(a1), dy1 = cosf(a1);
			// Band 0: solid (t=0.0..1.0, v=1..1)
			gaugeBuffer[N*0+i*4+0] = {  1.0f, 1.0f, dx0, dy0 };
			gaugeBuffer[N*0+i*4+1] = {  0.0f, 1.0f, dx0, dy0 };
			gaugeBuffer[N*0+i*4+2] = {  1.0f, 1.0f, dx1, dy1 };
			gaugeBuffer[N*0+i*4+3] = {  0.0f, 1.0f, dx1, dy1 };
			// Band 1: glow inner (t=-0.2..0.0, v=0..1)
			gaugeBuffer[N*4+i*4+0] = {  0.0f, 0.5f, dx0, dy0 };
			gaugeBuffer[N*4+i*4+1] = { -1.0f, 0.0f, dx0, dy0 };
			gaugeBuffer[N*4+i*4+2] = {  0.0f, 0.5f, dx1, dy1 };
			gaugeBuffer[N*4+i*4+3] = { -1.0f, 0.0f, dx1, dy1 };
			// Band 2: glow outer (t=1.0..1.2, v=1..0)
			gaugeBuffer[N*8+i*4+0] = {  2.0f, 0.0f, dx0, dy0 };
			gaugeBuffer[N*8+i*4+1] = {  1.0f, 0.5f, dx0, dy0 };
			gaugeBuffer[N*8+i*4+2] = {  2.0f, 0.0f, dx1, dy1 };
			gaugeBuffer[N*8+i*4+3] = {  1.0f, 0.5f, dx1, dy1 };
			/*
			// Band 0: glow inner (t=-0.2..0.0, v=0..1)
			gaugeBuffer[i*4+0]       = {  0.0f, 0.5f, dx0, dy0 };
			gaugeBuffer[i*4+1]       = { -1.5f, 0.0f, dx0, dy0 };
			gaugeBuffer[i*4+2]       = {  0.0f, 0.5f, dx1, dy1 };
			gaugeBuffer[i*4+3]       = { -1.5f, 0.0f, dx1, dy1 };
			// Band 1: solid (t=0.0..1.0, v=1..1)
			gaugeBuffer[N*4+i*4+0]   = {  1.0f, 1.0f, dx0, dy0 };
			gaugeBuffer[N*4+i*4+1]   = {  0.0f, 1.0f, dx0, dy0 };
			gaugeBuffer[N*4+i*4+2]   = {  1.0f, 1.0f, dx1, dy1 };
			gaugeBuffer[N*4+i*4+3]   = {  0.0f, 1.0f, dx1, dy1 };
			// Band 2: glow outer (t=1.0..1.2, v=1..0)
			gaugeBuffer[N*8+i*4+0]   = {  2.5f, 0.0f, dx0, dy0 };
			gaugeBuffer[N*8+i*4+1]   = {  1.0f, 0.5f, dx0, dy0 };
			gaugeBuffer[N*8+i*4+2]   = {  2.5f, 0.0f, dx1, dy1 };
			gaugeBuffer[N*8+i*4+3]   = {  1.0f, 0.5f, dx1, dy1 };
			*/
		}
		glGenBuffers( 1, &mGaugeVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mGaugeVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(gaugeBuffer), gaugeBuffer, GL_STATIC_DRAW );
	}

	glGenBuffers( 1, &mQuadVBO );
	glBindBuffer( GL_ARRAY_BUFFER, mQuadVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 6, nullptr, GL_STATIC_DRAW );

	setFont( "" );
}


RendererHUD::~RendererHUD()
{
}


void RendererHUD::setFont( const std::string& path )
{
	mFontPath = path;
	delete mDefaultFont;
	mDefaultFont = nullptr;

	if ( !mFontPath.empty() ) {
		mDefaultFont = new Font( mFontPath, mFontSize );
	} else {
		// Fallback: embedded font32 PNG atlas, 512x512, 16x16 grid of 32x32 cells
		Texture tmp;
		memset( &tmp, 0, sizeof(Texture) );
		struct membuf : std::streambuf {
			membuf( const uint8_t* b, const uint8_t* e ) { setg((char*)b,(char*)b,(char*)e); }
		};
		membuf sbuf( font32, font32 + sizeof(font32) );
		std::istream in_stream( &sbuf );
		LoadPNG( &tmp, in_stream );
		mDefaultFont = Font::fromRawPixels( tmp.data, tmp.width, tmp.height, tmp.width / 16, tmp.height / 16, mFontSize );
		free( tmp.data );
	}

	mDefaultFont->uploadGL( mFontSize * 0.35f );
}


Font* RendererHUD::loadFont( const std::string& path, uint32_t pixelSize )
{
	Font* f = new Font( path, pixelSize );
	f->uploadGL();
	return f;
}


void RendererHUD::PreRender()
{
	if ( Thread::GetSeconds() - mHUDTick >= 1.0f ) {
		mBlinkingViews = !mBlinkingViews;
		mHUDTick = Thread::GetSeconds();
	}
	glUseProgram( mFlatShader.mShader );
	glUniform1f( mFlatShader.mDistorID, (float)mBarrelCorrection );
	glUseProgram( mTextShader.mShader );
	glUniform1f( mTextShader.mDistorID, (float)mBarrelCorrection );
	glUseProgram( 0 );
}


void RendererHUD::RenderImage( int x, int y, int width, int height, uintptr_t img )
{
	Texture* tex = reinterpret_cast<Texture*>( img );
	RenderQuadTexture( tex->glID, x, y, width, height );
}



void RendererHUD::RenderQuadTexture( GLuint textureID, int x, int y, int width, int height, bool hmirror, bool vmirror, const Vector4f& color )
{
	glUseProgram( mFlatShader.mShader );
	glUniform4f( mFlatShader.mColorID, color.x, color.y, color.z, color.w );
	glUniform1f( mFlatShader.mScaleID, 1.0f );
	glBindTexture( GL_TEXTURE_2D, textureID );

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, textureID );

	if ( mNightMode ) {
		glUniform1f( mExposureID, 2.5f );
		glUniform1f( mGammaID, 1.5f );
	} else {
		glUniform1f( mExposureID, 1.0f );
		glUniform1f( mGammaID, 1.0f );
	}

	glBindBuffer( GL_ARRAY_BUFFER, mQuadVBO );
	glVertexAttribPointer( mFlatShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mFlatShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	float u = ( hmirror ? 1.0f : 0.0f );
	float v = ( vmirror ? 1.0f : 0.0f );
	float uend = ( hmirror ? 0.0f : 1.0f );
	float vend = ( vmirror ? 0.0f : 1.0f );

	FastVertex vertices[6];
	vertices[0].u = u;
	vertices[0].v = v;
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[1].u = uend;
	vertices[1].v = vend;
	vertices[1].x = x+width;
	vertices[1].y = y+height;
	vertices[2].u = uend;
	vertices[2].v = v;
	vertices[2].x = x+width;
	vertices[2].y = y;
	vertices[3].u = u;
	vertices[3].v = v;
	vertices[3].x = x;
	vertices[3].y = y;
	vertices[4].u = u;
	vertices[4].v = vend;
	vertices[4].x = x;
	vertices[4].y = y+height;
	vertices[5].u = uend;
	vertices[5].v = vend;
	vertices[5].x = x+width;
	vertices[5].y = y+height;
	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(FastVertex) * 6, vertices );

	glDrawArrays( GL_TRIANGLES, 0, 6 );
}


void RendererHUD::RenderText( int x, int y, const std::string& text, uint32_t color_, float size, TextAlignment halign, TextAlignment valign )
{
	Vector4f color = Vector4f( (float)( color_ & 0xFF ) / 256.0f, (float)( ( color_ >> 8 ) & 0xFF ) / 256.0f, (float)( ( color_ >> 16 ) & 0xFF ) / 256.0f, (float)( ( color_ >> 24 ) & 0xFF ) / 256.0f );
	RenderText( nullptr, x, y, text, color, size, halign, valign );
}


void RendererHUD::RenderText( int x, int y, const std::string& text, const Vector4f& color, float size, TextAlignment halign, TextAlignment valign, Vector4f outlineColor, Vector4f shadowColor )
{
	RenderText( nullptr, x, y, text, color, size, halign, valign, outlineColor, shadowColor );
}


void RendererHUD::RenderText( Font* font, int x, int y, const std::string& text, uint32_t color_, float size, TextAlignment halign, TextAlignment valign )
{
	Vector4f color = Vector4f( (float)( color_ & 0xFF ) / 256.0f, (float)( ( color_ >> 8 ) & 0xFF ) / 256.0f, (float)( ( color_ >> 16 ) & 0xFF ) / 256.0f, (float)( ( color_ >> 24 ) & 0xFF ) / 256.0f );
	RenderText( font, x, y, text, color, size, halign, valign );
}


void RendererHUD::RenderText( Font* font, int x, int y, const std::string& text, const Vector4f& _color, float size, TextAlignment halign, TextAlignment valign, Vector4f outlineColor, Vector4f shadowColor )
{
	Font* f = font ? font : mDefaultFont;
	if ( !f ) return;

	glUseProgram( mTextShader.mShader );
	Vector4f color = _color;
	if ( mNightMode ) {
		color.x *= 0.5f;
		color.y = std::min( 1.0f, color.y * 1.0f );
		color.z *= 0.5f;
	}
	glUniform1f( mTextShader.mScaleID, size );

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, f->glTextureID() );

	glBindBuffer( GL_ARRAY_BUFFER, f->glVBO() );
	glVertexAttribPointer( mTextShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof(FastVertex), (void*)0 );
	glVertexAttribPointer( mTextShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof(FastVertex), (void*)(sizeof(float)*2) );

	int tw, th, tyofs;
	f->measureText( text, &tw, &th, &tyofs, size );

	if ( halign == TextAlignment::CENTER ) {
		x -= tw / 2;
	} else if ( halign == TextAlignment::END ) {
		x -= tw;
	}

	if ( valign == TextAlignment::CENTER ) {
		y -= tyofs + th / 2;
	} else if ( valign == TextAlignment::END ) {
		y -= tyofs + th;
	}

	if ( shadowColor.w > 0.0f ) {
		float origX = x;
		glUniform4f( mTextShader.mColorID, 0.0f, 0.0f, 0.0f, 0.0f );
		glUniform4f( mTextOutlineColorID, 0.0f, 0.0f, 0.0f, 0.0f );
		glUniform4f( mTextShadowColorID, shadowColor.x, shadowColor.y, shadowColor.z, shadowColor.w );
		for ( uint32_t i = 0; i < text.length(); i++ ) {
			uint8_t c = (uint8_t)text[i];
			glUniform2f( mTextShader.mOffsetID, x, y );
			glDrawArrays( GL_TRIANGLES, c * 6, 6 );
			x += f->adv[c] * size;
		}
		x = origX;
	}
	glUniform4f( mTextShader.mColorID, color.x, color.y, color.z, color.w );
	glUniform4f( mTextOutlineColorID, outlineColor.x, outlineColor.y, outlineColor.z, outlineColor.w );
	glUniform4f( mTextShadowColorID, 0.0f, 0.0f, 0.0f, 0.0f );
	for ( uint32_t i = 0; i < text.length(); i++ ) {
		uint8_t c = (uint8_t)text[i];
		glUniform2f( mTextShader.mOffsetID, x, y );
		glDrawArrays( GL_TRIANGLES, c * 6, 6 );
		x += f->adv[c] * size;
	}
}


void RendererHUD::RenderRoundedRect( int x, int y, int width, int height, int radius, const Vector4f& bgColor, int strokeWidth, const Vector4f& strokeColor )
{
	glUseProgram( mRRectShader.mShader );
	glUniform2f( mRRectSizeID,    (float)width, (float)height );
	glUniform1f( mRRectRadiusID,  (float)radius );
	glUniform1f( mRRectStrokeWID, (float)strokeWidth );
	glUniform4f( mRRectShader.mColorID,  bgColor.x,     bgColor.y,     bgColor.z,     bgColor.w );
	glUniform4f( mRRectStrokeCID,        strokeColor.x, strokeColor.y, strokeColor.z, strokeColor.w );

	// Quad with texcoords [0,1] so the fragment shader gets normalised local coords
	FastVertex v[6] = {
		{ 0.0f, 0.0f,  (float)x,        (float)y         },
		{ 1.0f, 1.0f,  (float)(x+width), (float)(y+height) },
		{ 1.0f, 0.0f,  (float)(x+width), (float)y         },
		{ 0.0f, 0.0f,  (float)x,        (float)y         },
		{ 0.0f, 1.0f,  (float)x,        (float)(y+height) },
		{ 1.0f, 1.0f,  (float)(x+width), (float)(y+height) },
	};

	glBindBuffer( GL_ARRAY_BUFFER, mQuadVBO );
	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(v), v );
	glVertexAttribPointer( mRRectShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof(FastVertex), (void*)0 );
	glVertexAttribPointer( mRRectShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof(FastVertex), (void*)(sizeof(float)*2) );

	glDrawArrays( GL_TRIANGLES, 0, 6 );
}


void RendererHUD::RenderGauge( float cx, float cy, float radius, float inner_ratio, float angle_start, float angle_span, float fill, const Vector4f& color, const Vector4f& bgColor, const Vector4f& outerGlowColor, const Vector4f& innerGlowColor )
{
	fill  = std::max( 0.0f, std::min( 1.0f, fill ) );

	const int N = 128;
	int ofs         = (int)( angle_start / 360.0f * N + N ) % N;
	int count_total = (int)( angle_span / 360.0f * N );
	int count       = (int)( fill * angle_span / 360.0f * N );
	count_total     = std::max( std::min( count_total, N - ofs ), 0 );
	count           = std::max( std::min( count, N - ofs ), 0 );

	glUseProgram( mGaugeShader.mShader );
	glUniform2f( mGaugeCenterID,     cx, cy );
	glUniform1f( mGaugeRadiusID,     radius );
	glUniform1f( mGaugeInnerRatioID, inner_ratio );

	glBindBuffer( GL_ARRAY_BUFFER, mGaugeVBO );
	glVertexAttribPointer( mGaugeShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof(FastVertex), (void*)0 );
	glVertexAttribPointer( mGaugeShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof(FastVertex), (void*)( sizeof(float) * 2 ) );

	if ( bgColor.w > 0.0f ) {
		glUniform4f( mGaugeShader.mColorID, bgColor.x, bgColor.y, bgColor.z, bgColor.w );
		glUniform4f( mGaugeShader.mGlowColorID, 0.0f, 0.0f, 0.0f, 0.25f );
		glDrawArrays( GL_TRIANGLE_STRIP, ofs * 4, count_total * 4 );
		if ( innerGlowColor.w > 0.0f ) {
			glDrawArrays( GL_TRIANGLE_STRIP, N * 4 + ofs * 4, count_total * 4 );
		}
		if ( outerGlowColor.w > 0.0f ) {
			glDrawArrays( GL_TRIANGLE_STRIP, N * 8 + ofs * 4, count_total * 4 );
		}
	}

	// Filled arc
	glUniform4f( mGaugeShader.mColorID, color.x, color.y, color.z, color.w );
	glDrawArrays( GL_TRIANGLE_STRIP, ofs * 4, count * 4 );
	if ( innerGlowColor.w > 0.0f ) {
		glUniform4f( mGaugeShader.mGlowColorID, innerGlowColor.x, innerGlowColor.y, innerGlowColor.z, innerGlowColor.w );
		glDrawArrays( GL_TRIANGLE_STRIP, N * 4 + ofs * 4, count * 4 );
	}
	if ( outerGlowColor.w > 0.0f ) {
		glUniform4f( mGaugeShader.mGlowColorID, outerGlowColor.x, outerGlowColor.y, outerGlowColor.z, outerGlowColor.w );
		glDrawArrays( GL_TRIANGLE_STRIP, N * 8 + ofs * 4, count * 4 );
	}
}


void RendererHUD::createPipeline( RendererHUD::Shader* target )
{
	if ( target->mShader ) {
		glDeleteProgram( target->mShader );
	}
	target->mShader = glCreateProgram();
	glAttachShader( target->mShader, target->mVertexShader );
	glAttachShader( target->mShader, target->mFragmentShader );

	glLinkProgram( target->mShader );

	target->mVertexTexcoordID = glGetAttribLocation( target->mShader, "ge_VertexTexcoord" );
	target->mVertexColorID = glGetAttribLocation( target->mShader, "ge_VertexColor" );
	target->mVertexPositionID = glGetAttribLocation( target->mShader, "ge_VertexPosition" );

	char log[4096] = "";
	int logsize = 4096;
	glGetProgramInfoLog( target->mShader, logsize, &logsize, log );
	std::cout << "program compile : " << log << "\n";

	glUseProgram( target->mShader );
	target->mMatrixProjectionID = glGetUniformLocation( target->mShader, "ge_ProjectionMatrix" );
	target->mDistorID = glGetUniformLocation( target->mShader, "distort" );
	target->mColorID = glGetUniformLocation( target->mShader, "color" );
	target->mOffsetID = glGetUniformLocation( target->mShader, "offset" );
	target->mScaleID = glGetUniformLocation( target->mShader, "scale" );
	glUniformMatrix4fv( target->mMatrixProjectionID, 1, GL_FALSE, mMatrixProjection->constData() );
}


int RendererHUD::LoadVertexShader( Shader* target, const void* data, size_t size )
{
	if ( target->mVertexShader ) {
		glDeleteShader( target->mVertexShader );
	}

	target->mVertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( target->mVertexShader, 1, (const char**)&data, NULL );
	glCompileShader( target->mVertexShader );
	char log[4096] = "";
	int logsize = 4096;
	glGetShaderInfoLog( target->mVertexShader, logsize, &logsize, log );
	std::cout << "vertex compile : " << log << "\n";

	return 0;
}


int RendererHUD::LoadFragmentShader( Shader* target, const void* data, size_t size )
{
	if ( target->mFragmentShader ) {
		glDeleteShader( target->mFragmentShader );
	}

	target->mFragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( target->mFragmentShader, 1, (const char**)&data, NULL );
	glCompileShader( target->mFragmentShader );
	char log[4096] = "";
	int logsize = 4096;
	glGetShaderInfoLog( target->mFragmentShader, logsize, &logsize, log );
	std::cout << "fragment compile : " << log << "\n";

	return 0;
}


void RendererHUD::FontMeasureString( const std::string& str, int* width, int* height )
{
	int i = 0;
	int mx = 0;
	int x = 0;
	int y = 0;

	for ( i = 0; str[i]; i++ ) {
		if ( str[i] == '\n' ) {
			if ( x > mx ) {
				mx = x;
			}
			x = 0;
			y += mFontSize;
			continue;
		}
		x += mDefaultFont ? mDefaultFont->adv[ (uint8_t)str[i] ] : 0;
	}
	if ( x > mx ) {
		mx = x;
	}
	if ( mx == 0 ) {
		mx = x;
	}

	*width = mx;
	*height = y + mFontSize + ( mFontSize * 0.4 );
}


int32_t RendererHUD::CharacterWidth( Texture* tex, unsigned char c )
{
	uint32_t xbase = ( tex->width / 16 ) * ( c % 16 );
	uint32_t ybase = ( tex->height / 16 ) * ( c / 16 );
	uint32_t xend = xbase + tex->width / 16;
	uint32_t yend = ybase + tex->height / 16;
	uint32_t xfirst = 0;
	uint32_t xlast = 0;

	for ( uint32_t y = ybase; y < yend; y++ ) {
		for ( uint32_t x = xbase; x < xend; x++ ) {
			if ( tex->data[x + y*tex->width] != 0x00000000 ) {
				if ( xfirst == 0 or x < xfirst ) {
					xfirst = x;
				}
				if ( x + 1 < xend and tex->data[x+1 + y*tex->width] == 0x00000000 ){
					if ( xfirst != 0 and x > xlast ) {
						xlast = x;
					}
				}
			}
		}
	}

	if ( xfirst > 0 and xlast > 0 ) {
		return xlast - xfirst + 1;
	}
	return tex->width / 16;
}


int32_t RendererHUD::CharacterHeight( Texture* tex, unsigned char c )
{
	// TODO : calculate fine value
	return tex->height / 16;
}


int32_t RendererHUD::CharacterYOffset( Texture* tex, unsigned char c )
{
	uint32_t xbase = ( tex->width / 16 ) * ( c % 16 );
	uint32_t ybase = ( tex->height / 16 ) * ( c / 16 );
	uint32_t xend = xbase + tex->width / 16;
	uint32_t yend = ybase + tex->height / 16;

	for ( uint32_t y = ybase; y < yend; y++ ) {
		for ( uint32_t x = xbase; x < xend; x++ ) {
			if ( tex->data[x + y*tex->width] != 0x00000000 ) {
				return y - ybase;
			}
		}
	}

	return 0;
}
	std::istringstream data(std::string(font32));


RendererHUD::Texture* RendererHUD::LoadTexture( const std::string& filename )
{
	std:cout << "LoadTexture( \"" << filename << "\" )\n";
	std::string type = "none";
	const char* dot = strrchr( filename.c_str(), '.' );
	if ( dot ) {
		if ( strcmp( dot, ".png" ) == 0 or strcmp( dot, ".PNG" ) == 0 ) {
			type = "png";
		}
	}
	if ( type == "none" ) {
		return nullptr;
	}

	Texture* tex = new Texture;
	memset( tex, 0, sizeof(Texture) );

	if ( type == "png" ) {
		std::ifstream file( filename, std::ios_base::in | std::ios_base::binary );
		LoadPNG( tex, file );
		file.close();
	}

	glActiveTexture( GL_TEXTURE0 );
	glGenTextures( 1, &tex->glID );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, tex->glID );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->data );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	return tex;
}


RendererHUD::Texture* RendererHUD::LoadTexture( const uint8_t* data, uint32_t datalen )
{
	Texture* tex = new Texture;
	memset( tex, 0, sizeof(Texture) );

	struct membuf : std::streambuf {
		membuf( const uint8_t* begin, const uint8_t* end ) {
			this->setg( (char*)begin, (char*)begin, (char*)end );
		}
	};
	membuf sbuf( data, data+datalen );
	std::istream in_stream( &sbuf );
	LoadPNG( tex, in_stream );

	glActiveTexture( GL_TEXTURE0 );
	glGenTextures( 1, &tex->glID );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, tex->glID );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->data );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	return tex;
}


static void png_read_from_ifstream( png_structp png_ptr, png_bytep data, png_size_t length )
{
	std::istream* file = ( std::istream* )png_get_io_ptr( png_ptr );
	file->read( (char*)data, length );
}


void RendererHUD::LoadPNG( Texture* tex, std::istream& file )
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height, x, y;
	int bit_depth, color_type, interlace_type;
	uint32_t* line;

// 	std::ifstream file( filename, std::ios_base::in | std::ios_base::binary );
/*
#ifdef png_check_sig
	uint8_t magic[8] = { 0x0 };
	file.read( (char*)magic, 8 );
	file.seekg( 0, file.beg );
	if ( !png_check_sig( magic, 8 ) ) {
		return;
	}
#endif
*/
	std::cout << "libpng version : " << PNG_LIBPNG_VER_STRING << "\n";

	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );
	if ( png_ptr == nullptr ) {
		std::cout << "PNG Error: png_create_read_struct returned null\n";
		return;
	}

	png_set_error_fn( png_ptr, nullptr, nullptr, nullptr );
	png_set_read_fn( png_ptr, ( png_voidp* )&file, png_read_from_ifstream );

	if ( ( info_ptr = png_create_info_struct( png_ptr ) ) == nullptr ) {
		png_destroy_read_struct( &png_ptr, nullptr, nullptr );
		return;
	}

//	png_read_png( png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr );
	png_read_info( png_ptr, info_ptr );

	png_get_IHDR( png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, nullptr, nullptr );
	tex->width = width;
	tex->height = height;

	tex->data = ( uint32_t* ) malloc( tex->width * tex->height * sizeof( uint32_t ) );
	if ( !tex->data ) {
		free( tex->data );
		png_destroy_read_struct( &png_ptr, nullptr, nullptr );
		return;
	}

	png_set_strip_16( png_ptr );
	png_set_packing( png_ptr );
	if ( color_type == PNG_COLOR_TYPE_PALETTE ) png_set_palette_to_rgb( png_ptr );
	if ( png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS ) ) png_set_tRNS_to_alpha( png_ptr );
	png_set_filler( png_ptr, 0xff, PNG_FILLER_AFTER );

	line = ( uint32_t* ) malloc( width * sizeof( uint32_t ) );
	if ( !line ) {
		free( tex->data );
		png_destroy_read_struct( &png_ptr, nullptr, nullptr );
		return;
	}

	for ( y = 0; y < height; y++ ) {
		png_read_row( png_ptr, (uint8_t*)line, nullptr );
		for ( x = 0; x < width; x++ ) {
			uint32_t color = line[x];
			tex->data[ x + y * tex->width] = color;
		}
	}

	free( line );
	png_read_end( png_ptr, info_ptr );
	png_destroy_read_struct( &png_ptr, &info_ptr, nullptr );
}
