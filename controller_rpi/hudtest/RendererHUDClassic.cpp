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

#include <cmath>
#include <gammaengine/File.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include "RendererHUDClassic.h"


static const char hud_vertices_shader[] =
R"(	#define in attribute
	#define out varying
	#define ge_Position gl_Position
	in vec2 ge_VertexTexcoord;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2 offset;

	void main()
	{
		vec2 pos = offset + ge_VertexPosition.xy;
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
		ge_Position.y = ge_Position.y * ( 720.0 / ( 1280.0 / 2.0 ) );
	})"
;

static const char hud_fragment_shader[] =
R"(	#define in varying
	#define ge_FragColor gl_FragColor

	uniform vec4 color;

	void main()
	{
		ge_FragColor = color;
	})"
;


static const char hud_color_vertices_shader[] =
R"(	#define in attribute
	#define out varying
	#define ge_Position gl_Position
	in vec2 ge_VertexTexcoord;
	in vec4 ge_VertexColor;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2 offset;
	out vec4 ge_Color;

	void main()
	{
		ge_Color = ge_VertexColor;
		vec2 pos = offset + ge_VertexPosition.xy;
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
		ge_Position.y = ge_Position.y * ( 720.0 / ( 1280.0 / 2.0 ) );
	})"
;

static const char hud_color_fragment_shader[] =
R"(	#define in varying
	#define ge_FragColor gl_FragColor

	in vec4 ge_Color;

	void main()
	{
		ge_FragColor = ge_Color;
	})"
;


RendererHUDClassic::RendererHUDClassic( Instance* instance, Font* font )
	: RendererHUD( instance, font )
	, mSmoothRPY( Vector3f() )
	, mShader{ 0 }
	, mColorShader{ 0 }
{
	LoadVertexShader( &mShader, hud_vertices_shader, sizeof(hud_vertices_shader) + 1 );
	LoadFragmentShader( &mShader, hud_fragment_shader, sizeof(hud_fragment_shader) + 1 );
	createPipeline( &mShader );
	glUseProgram( mShader.mShader );
	glEnableVertexAttribArray( mShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mShader.mVertexPositionID );
	glUseProgram( 0 );

	LoadVertexShader( &mColorShader, hud_color_vertices_shader, sizeof(hud_color_vertices_shader) + 1 );
	LoadFragmentShader( &mColorShader, hud_color_fragment_shader, sizeof(hud_color_fragment_shader) + 1 );
	createPipeline( &mColorShader );
	glUseProgram( mColorShader.mShader );
	glEnableVertexAttribArray( mColorShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mColorShader.mVertexColorID );
	glEnableVertexAttribArray( mColorShader.mVertexPositionID );
	glUseProgram( 0 );

	Compute();
}


RendererHUDClassic::~RendererHUDClassic()
{
}


void RendererHUDClassic::Render( GE::Window* window, Controller* controller, VideoStats* videostats, IwStats* iwstats )
{
}


void RendererHUDClassic::RenderThrust( float thrust )
{
	glUseProgram( mShader.mShader );
	glUniform4f( mShader.mColorID, 0.65f, 1.0f, 0.75f, 1.0f );

	glBindBuffer( GL_ARRAY_BUFFER, mThrustVBO );
	glVertexAttribPointer( mShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	glUniform2f( mShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 32, (int)( thrust * 32.0f ) ) );
	glUniform2f( mShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 32, (int)( thrust * 32.0f ) ) );
}


void RendererHUDClassic::RenderLink( float quality )
{
	glUseProgram( mColorShader.mShader );
// 	glUniform4f( mColorShader.mColorID, 0.65f, 1.0f, 0.75f, 1.0f );
	glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
	glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
	glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
	glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

	glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( quality * 16.0f ) ) );
	glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( quality * 16.0f ) ) );
}


void RendererHUDClassic::RenderBattery( float level )
{
	glUseProgram( mShader.mShader );
	glUniform4f( mShader.mColorID, 1.0f - level * 0.5f, level, level * 0.5f, 1.0f );

	int ofsy = (int)( level * 96.0f );
	FastVertex batteryBuffer[6];
	float x = 1280.0f * ( 1.0f / 4.0f + 0.6f * 1.0f / 4.0f );
	float y = 720.0f - 75.0f;
	batteryBuffer[0].x = x;
	batteryBuffer[0].y = y-ofsy;
	batteryBuffer[1].x = x+32;
	batteryBuffer[1].y = y;
	batteryBuffer[2].x = x+32;
	batteryBuffer[2].y = y-ofsy;
	batteryBuffer[3].x = x;
	batteryBuffer[3].y = y-ofsy;
	batteryBuffer[4].x = x;
	batteryBuffer[4].y = y;
	batteryBuffer[5].x = x+32;
	batteryBuffer[5].y = y;
	for ( int j = 0; j < 6; j++ ) {
		Vector2f vec = VR_Distort( Vector2f( batteryBuffer[j].x, batteryBuffer[j].y ) );
		batteryBuffer[j].x = vec.x;
		batteryBuffer[j].y = vec.y;
	}
	glBindBuffer( GL_ARRAY_BUFFER, mBatteryVBO );
	glBufferSubData( GL_ARRAY_BUFFER, 0, 6 * sizeof(FastVertex), batteryBuffer );

	glVertexAttribPointer( mShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	glUniform2f( mShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	glUniform2f( mShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
}


void RendererHUDClassic::RenderAttitude( const Vector3f& rpy )
{
	mSmoothRPY = mSmoothRPY + ( rpy - mSmoothRPY ) * 35.0f * ( 1.0f / 60.0f );
	if ( std::abs( rpy.x ) <= 0.25f ) {
		mSmoothRPY.x = 0.0f;
	}
	if ( std::abs( rpy.y ) <= 0.25f ) {
		mSmoothRPY.y = 0.0f;
	}

	glUseProgram( mColorShader.mShader );
	uint32_t color = 0xFFFFFFFF;
	if ( mNightMode ) {
		color = 0xFF40FF40;
	}

	glLineWidth( 1.0f );

	glBindBuffer( GL_ARRAY_BUFFER, mLineVBO );
	glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
	glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
	glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

	float xrot = M_PI * ( -mSmoothRPY.x / 180.0f );
	float yofs = ( 720.0f / 2.0f ) * ( -mSmoothRPY.y / 180.0f ) * M_PI * 2.75f;
	float s = std::sin( xrot );
	float c = std::cos( xrot );

	FastVertexColor linesBuffer[12];
	for ( uint32_t j = 0; j < sizeof(linesBuffer)/sizeof(FastVertexColor); j++ ) {
		float xpos = (float)j / (float)(sizeof(linesBuffer)/sizeof(FastVertexColor));
		Vector2f p;
		Vector2f p0 = Vector2f( ( xpos - 0.5f ) * ( 1280.0f / 2.0f ), yofs );
		p.x = 1280.0f / 4.0f + p0.x * c - p0.y * s;
		p.y = 720.0f / 2.0f + p0.x * s + p0.y * c;
		p = VR_Distort( p );
		linesBuffer[j].x = p.x;
		linesBuffer[j].y = p.y;
		linesBuffer[j].color = color;
		if ( xpos <= 0.1f or xpos >= 0.9f ) {
			linesBuffer[j].color = 0x7F000000 | ( color & 0x00FFFFFF );
		}
	}

	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(linesBuffer), linesBuffer );
	glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
	glDrawArrays( GL_LINE_STRIP, 0, sizeof(linesBuffer)/sizeof(FastVertexColor) );
	glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_LINE_STRIP, 0, sizeof(linesBuffer)/sizeof(FastVertexColor) );
}


void RendererHUDClassic::Compute()
{
	{
		FastVertex thrustBuffer[6 * 32];
		memset( thrustBuffer, 0, sizeof( thrustBuffer ) );
		for ( uint32_t i = 0; i < 32; i++ ) {
			float height = std::exp( i * 0.05f );
			int ofsy = height * 10.0f;
			float x = 50 + i * ( 5 + 1 );
			float y = 720 - 75 - ofsy;
			
			thrustBuffer[i*6 + 0].x = x;
			thrustBuffer[i*6 + 0].y = y;
			thrustBuffer[i*6 + 1].x = x+5;
			thrustBuffer[i*6 + 1].y = y+ofsy;
			thrustBuffer[i*6 + 2].x = x+5;
			thrustBuffer[i*6 + 2].y = y;
			thrustBuffer[i*6 + 3].x = x;
			thrustBuffer[i*6 + 3].y = y;
			thrustBuffer[i*6 + 4].x = x;
			thrustBuffer[i*6 + 4].y = y+ofsy;
			thrustBuffer[i*6 + 5].x = x+5;
			thrustBuffer[i*6 + 5].y = y+ofsy;

			for ( int j = 0; j < 6; j++ ) {
				Vector2f vec = VR_Distort( Vector2f( thrustBuffer[i*6 + j].x, thrustBuffer[i*6 + j].y ) );
				thrustBuffer[i*6 + j].x = vec.x;
				thrustBuffer[i*6 + j].y = vec.y;
			}
		}
		glGenBuffers( 1, &mThrustVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mThrustVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 6 * 32, thrustBuffer, GL_STATIC_DRAW );
	}

	{
		FastVertexColor linkBuffer[6 * 16];
		memset( linkBuffer, 0, sizeof( linkBuffer ) );
		for ( uint32_t i = 0; i < 16; i++ ) {
			float height = std::exp( i * 0.1f );
			int ofsy = height * 12.0f;
			float x = 1280.0f * 0.03f + i * ( 13 + 1 );
			float y = 145.0f;

			linkBuffer[i*6 + 0].x = x;
			linkBuffer[i*6 + 0].y = y-ofsy;
			linkBuffer[i*6 + 1].x = x+13;
			linkBuffer[i*6 + 1].y = y;
			linkBuffer[i*6 + 2].x = x+13;
			linkBuffer[i*6 + 2].y = y-ofsy;
			linkBuffer[i*6 + 3].x = x;
			linkBuffer[i*6 + 3].y = y-ofsy;
			linkBuffer[i*6 + 4].x = x;
			linkBuffer[i*6 + 4].y = y;
			linkBuffer[i*6 + 5].x = x+13;
			linkBuffer[i*6 + 5].y = y;
			Vector4f fcolor = Vector4f( 0.5f + 0.5f * 0.0625f * ( 15 - i ), 0.5f + 0.5f * 0.0625f * i, 0.5f, 1.0f );
			uint32_t color = 0xFF7F0000 | ( (uint32_t)( fcolor.y * 255.0f ) << 8 ) | ( (uint32_t)( fcolor.x * 255.0f ) );
			linkBuffer[i*6 + 0].color = linkBuffer[i*6 + 1].color = linkBuffer[i*6 + 2].color = linkBuffer[i*6 + 3].color = linkBuffer[i*6 + 4].color = linkBuffer[i*6 + 5].color = color;

			for ( int j = 0; j < 6; j++ ) {
				Vector2f vec = VR_Distort( Vector2f( linkBuffer[i*6 + j].x, linkBuffer[i*6 + j].y ) );
				linkBuffer[i*6 + j].x = vec.x;
				linkBuffer[i*6 + j].y = vec.y;
			}
		}
		glGenBuffers( 1, &mLinkVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertexColor) * 6 * 16, linkBuffer, GL_STATIC_DRAW );
	}

	{
		glGenBuffers( 1, &mBatteryVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mBatteryVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 6, nullptr, GL_DYNAMIC_DRAW );
	}

	{
		glGenBuffers( 1, &mLineVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mLineVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertexColor) * 2 * 128, nullptr, GL_DYNAMIC_DRAW );
	}
}
