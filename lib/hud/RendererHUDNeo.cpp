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

/* TODO Add :
 * picture taken indicator
 * altitude [altitude hold]
*/

#include <cmath>
#include <sstream>
#include "RendererHUDNeo.h"

#define GL_GLEXT_PROTOTYPES
#include <string.h>
#include <Thread.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


static const char hud_vertices_shader[] =
R"(	#version 100
	precision mediump float;

	#define in attribute
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
// 		ge_Position.y = ge_Position.y * ( 720.0 / ( 1280.0 / 2.0 ) );
	})"
;

static const char hud_fragment_shader[] =
R"(	#version 100
	precision mediump float;

	#define in varying
	#define ge_FragColor gl_FragColor

	uniform vec4 color;

	void main()
	{
		ge_FragColor = color;
	})"
;


static const char hud_color_vertices_shader[] =
R"(	#version 100
	precision mediump float;

	#define in attribute
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
// 		ge_Position.y = ge_Position.y * ( 720.0 / ( 1280.0 / 2.0 ) );
	})"
;

static const char hud_color_fragment_shader[] =
R"(	#version 100
	precision mediump float;

	#define in varying
	#define ge_FragColor gl_FragColor

	uniform vec4 color;
	in vec4 ge_Color;

	void main()
	{
		ge_FragColor = color * ge_Color;
	})"
;


RendererHUDNeo::RendererHUDNeo( int width, int height, float ratio, uint32_t fontsize, Vector4i render_region, bool barrel_correction )
	: RendererHUD( width, height, ratio, fontsize, render_region, barrel_correction )
	, mSmoothRPY( Vector3f() )
	, mShader{ 0 }
	, mColorShader{ 0 }
	, mSpeedometerSize( 65 * fontsize / 28 )
{
	// mIconNight = LoadTexture( "data/icon_night.png" );
	// mIconPhoto = LoadTexture( "data/icon_photo.png" );

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


RendererHUDNeo::~RendererHUDNeo()
{
}


void RendererHUDNeo::Render( DroneStats* dronestats, float localVoltage, VideoStats* videostats, LinkStats* iwstats )
{
	if ( dronestats and dronestats->username != "" ) {
		RenderText( mWidth * 0.5f, mBorderTop, dronestats->username, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 1.0f, TextAlignment::CENTER );
	}

	// Controls
	if ( dronestats ) {
		float thrust = dronestats->thrust;
		if ( not dronestats->armed ) {
			thrust = 0.0f;
		}
		float acceleration = dronestats->acceleration / 9.7f;
		// if ( dronestats->mode != DroneMode::Rate ) {
			RenderAttitude( dronestats->rpy );
		// }
		RenderThrustAcceleration( thrust, acceleration / 5.0f );

		DroneMode mode = dronestats->mode;
		Vector4f color_acro = Vector4f( 0.4f, 0.4f, 0.4f, 1.0f );
		Vector4f color_hrzn = Vector4f( 0.4f, 0.4f, 0.4f, 1.0f );
		if ( mode == DroneMode::Rate ) {
			color_acro.x = color_acro.y = color_acro.z = 1.0f;
		} else if ( mode == DroneMode::Stabilize ) {
			color_hrzn.x = color_hrzn.y = color_hrzn.z = 1.0f;
		}
		int w = 0, h = 0;
		FontMeasureString( "ACRO/HRZN", &w, &h );
		int mode_x = mBorderRight - mSpeedometerSize * 0.7f - w * 0.45f * 0.5f;
		RenderText( mode_x, mBorderBottom - mSpeedometerSize * 0.29f, "ACRO", color_acro, 0.45f );
		FontMeasureString( "ACRO", &w, &h );
		RenderText( mode_x + w * 0.45f, mBorderBottom - mSpeedometerSize * 0.29f, "/", Vector4f( 0.65f, 0.65f, 0.65f, 1.0f ), 0.45f );
		FontMeasureString( "ACRO/", &w, &h );
		RenderText( mode_x + w * 0.45f, mBorderBottom - mSpeedometerSize * 0.29f, "HRZN", color_hrzn, 0.45f );

		Vector4f color = Vector4f( 1.0f, 1.0f, 1.0f, 1.0f );
		std::string saccel = std::to_string( acceleration );
		saccel = saccel.substr( 0, saccel.find( "." ) + 2 );
		RenderText( mBorderRight - mSpeedometerSize * 0.7f, mBorderBottom - mSpeedometerSize * 0.575f, saccel + "g", color, 0.55f, TextAlignment::CENTER );
	
		if ( dronestats->armed ) {
			RenderText( mBorderRight - mSpeedometerSize * 0.7f, mBorderBottom - mSpeedometerSize * 0.75f - mFontSize * 0.6f, std::to_string( (int)( thrust * 100.0f ) ) + "%", color, 1.0f, TextAlignment::CENTER );
		} else {
			RenderText( mBorderRight - mSpeedometerSize * 0.7f, mBorderBottom - mSpeedometerSize * 0.75f - mFontSize * 0.6f * 0.6f, "disarmed", color, 0.6f, TextAlignment::CENTER );
		}
	}

	int bottom_left_text_idx = 0;

	// Battery
	if ( dronestats ) {
		float level = dronestats->batteryLevel;
		RenderBattery( level );
		if ( level <= 0.0f and mBlinkingViews ) {
			RenderText( mWidth * 0.5f, mBorderBottom - mHeight * 0.15f, "Battery dead", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, TextAlignment::CENTER );
		} else if ( level <= 0.15f and mBlinkingViews ) {
			RenderText( mWidth * 0.5f, mBorderBottom - mHeight * 0.15f, "Battery critical", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, TextAlignment::CENTER );
		} else if ( level <= 0.25f and mBlinkingViews ) {
			RenderText( mWidth * 0.5f, mBorderBottom - mHeight * 0.15f, "Battery low", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, TextAlignment::CENTER );
		}
		float battery_red = 1.0f - level;
		RenderText( mBorderLeft + 210, mBorderBottom - mFontHeight * 1, std::to_string( (int)( level * 100.0f ) ) + "%", Vector4f( 0.5f + 0.5f * battery_red, 1.0f - battery_red * 0.25f, 0.5f - battery_red * 0.5f, 1.0f ), 0.9 );
		std::string svolt = std::to_string( dronestats->batteryVoltage );
		svolt = svolt.substr( 0, svolt.find( "." ) + 3 );
		RenderText( mBorderLeft, mBorderBottom - mFontHeight * 3, svolt + "V", Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.75f );
		RenderText( mBorderLeft, mBorderBottom - mFontHeight * 2, std::to_string( dronestats->batteryTotalCurrent ) + "mAh", Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.9f );

		bottom_left_text_idx = 4;

		if ( localVoltage > 0.0f ) {
			if ( localVoltage <= 11.0f and mBlinkingViews ) {
				RenderText( mWidth * 0.5f, mBorderBottom - mHeight * 0.15f - mFontSize, "Low Goggles Battery", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, TextAlignment::CENTER );
			}
			battery_red = 1.0f - ( ( localVoltage - 11.0f ) / 1.6f );
			svolt = std::to_string( localVoltage );
			svolt = svolt.substr( 0, svolt.find( "." ) + 3 );
			RenderText( mBorderLeft, mBorderBottom - mFontHeight * 4, svolt + "V", Vector4f( 0.5f + 0.5f * battery_red, 1.0f - battery_red * 0.25f, 0.5f, 1.0f ), 0.75 );

			bottom_left_text_idx++;
		}
	}

	// Blackbox and stats
	if ( dronestats ) {
		RenderText( mBorderLeft, mBorderBottom - mFontHeight * bottom_left_text_idx, "BB " + std::to_string(dronestats->blackBoxId), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.75f );
		bottom_left_text_idx++;
		RenderText( mBorderLeft, mBorderBottom - mFontHeight * bottom_left_text_idx, "CPU " + std::to_string(dronestats->cpuUsage) + "%", Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.75f );
		bottom_left_text_idx++;
		RenderText( mBorderLeft, mBorderBottom - mFontHeight * bottom_left_text_idx, "MEM " + std::to_string(dronestats->memUsage) + "%", Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.75f );
		bottom_left_text_idx++;
		if ( dronestats->memUsage > 75 ) {
			RenderText( mWidth * 0.5f, mBorderBottom - mHeight * 0.15f + mFontHeight, "High memory usage", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, TextAlignment::CENTER );
		} else if ( dronestats->cpuUsage > 75 ) {
			RenderText( mWidth * 0.5f, mBorderBottom - mHeight * 0.15f + mFontHeight, "High CPU usage", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, TextAlignment::CENTER );
		}
	}

	// Static elements
	if ( dronestats ) {
		glUseProgram( mColorShader.mShader );
		if ( mNightMode ) {
			glUniform4f( mColorShader.mColorID, 0.4f, 1.0f, 0.4f, 1.0f );
		} else {
			glUniform4f( mColorShader.mColorID, 1.0f, 1.0f, 1.0f, 1.0f );
		}
		// Lines
		{
			glLineWidth( 2.0f );

			glBindBuffer( GL_ARRAY_BUFFER, mStaticLinesVBO );
			glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
			glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
			glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

			DrawArrays( mColorShader, GL_LINES, 0, ( dronestats->mode == DroneMode::Rate ) ? mStaticLinesCountNoAttitude : mStaticLinesCount );
		}
		// Circle
		{
			glLineWidth( 2.0f );

			glBindBuffer( GL_ARRAY_BUFFER, mCircleVBO );
			glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
			glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
			glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );
/*
			glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
			if ( dronestats->mode != DroneMode::Rate ) {
				glDrawArrays( GL_LINE_STRIP, 0, 64 );
				glDrawArrays( GL_LINE_STRIP, 64, 64 );
			}
			glDrawArrays( GL_LINE_STRIP, 128, 65 );
			if ( mStereo ) {
				glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
				if ( dronestats->mode != DroneMode::Rate ) {
					glDrawArrays( GL_LINE_STRIP, 0, 64 );
					glDrawArrays( GL_LINE_STRIP, 64, 64 );
				}
				glDrawArrays( GL_LINE_STRIP, 128, 65 );
			}
*/
			if ( dronestats->mode != DroneMode::Rate ) {
				DrawArrays( mColorShader, GL_LINE_STRIP, 0, 64 );
				DrawArrays( mColorShader, GL_LINE_STRIP, 64, 64 );
			}
			DrawArrays( mColorShader, GL_LINE_STRIP, 128, 65 );
		}
	}

	// Link status
	if ( iwstats ) {
		float level = ( iwstats->level != -200 ) ? ( ( (float)( 100 + iwstats->level ) * 100.0f / 50.0f ) / 100.0f ) : 0.8f;
		RenderLink( (float)iwstats->qual / 100.0f, level );
		float link_red = 1.0f - ((float)iwstats->qual) / 100.0f;
		std::string channel_str = ( iwstats->channel > 400 ) ? ( std::to_string( iwstats->channel ) + "MHz" ) : ( "Ch" + std::to_string( iwstats->channel ) );
		std::string level_str = ( iwstats->level != -200 ) ? ( std::to_string( iwstats->level ) + "dBm  " ) : "";
		std::string link_quality_str = channel_str + "  " + level_str + std::to_string( iwstats->qual ) + "%";
		RenderText( mBorderLeft, mBorderTop + 60, link_quality_str, Vector4f( 0.5f + 0.5f * link_red, 1.0f - link_red * 0.25f, 0.5f - link_red * 0.5f, 1.0f ), 0.9f );
	}

	// video-infos + FPS
	if ( videostats ) {
		int w = 0, h = 0;
		std::string res_str = std::to_string( videostats->width ) + "x" + std::to_string( videostats->height );
		FontMeasureString( res_str, &w, &h );
		RenderText( (float)mBorderRight - w*0.775f, mBorderTop, res_str, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.8f );

		if ( videostats->fps != 0 ) {
			float fps_red = std::max( 0.0f, std::min( 1.0f, ((float)( 50 - videostats->fps ) ) / 50.0f ) );
			std::string fps_str = std::to_string( videostats->fps ) + "fps";
			FontMeasureString( fps_str, &w, &h );
			RenderText( (float)mBorderRight - w*0.775f, mBorderTop + mFontHeight * 1.0f, fps_str, Vector4f( 0.5f + 0.5f * fps_red, 1.0f - fps_red * 0.25f, 0.5f - fps_red * 0.5f, 1.0f ), 0.8f );
		}

		FontMeasureString( videostats->whitebalance, &w, &h );
		RenderText( (float)mBorderRight - w*0.775f, mBorderTop + mFontHeight * 3.0f, videostats->whitebalance, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.8f );
		FontMeasureString( videostats->exposure, &w, &h );
		RenderText( (float)mBorderRight - w*0.775f, mBorderTop + mFontHeight * 4.0f, videostats->exposure, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.8f );
/*
		if ( mNightMode ) {
			RenderQuadTexture( mIconNight->glID, mBorderRight - mWidth * 0.04f, mHeight / 2 - mHeight * 0.15, mWidth * 0.04f, mWidth * 0.035f, false, false, { 1.0f, 1.0f, 1.0f, 1.0f } );
		} else {
			RenderQuadTexture( mIconNight->glID, mBorderRight - mWidth * 0.04f, mHeight / 2 - mHeight * 0.15, mWidth * 0.04f, mWidth * 0.035f, false, false, { 0.5f, 0.5f, 0.5f, 0.5f } );
		}
*/
/*
		float photo_alpha = 0.5f;
		float photo_burn = 1.0f;
		if ( videostats->photo_id != mLastPhotoID ) {
			mLastPhotoID = videostats->photo_id;
			mPhotoTick = Thread::GetSeconds();
		}
		if ( mPhotoTick != 0.0f and Thread::GetSeconds() - mPhotoTick < 0.5f ) {
			float diff = Thread::GetSeconds() - mPhotoTick;
			photo_alpha += 0.75f * 2.0f * ( 0.5f - diff );
			photo_burn += 2.0f * 2.0f * ( 0.5f - diff );
		}
		RenderQuadTexture( mIconPhoto->glID, mBorderRight - mWidth * 0.04f, mHeight / 2 - mHeight * 0.07, mWidth * 0.04f, mWidth * 0.035f, false, false, { photo_burn, photo_burn, photo_burn, photo_alpha } );
*/
		if ( videostats->vtxFrequency > 0 ) {
			std::string text = std::to_string(videostats->vtxFrequency) + "MHz";
			FontMeasureString( text, &w, &h );
			RenderText( (float)mBorderRight - w*0.775f, mBorderTop + mFontHeight * 5.0f, text, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.8f );
		}
		if ( videostats->vtxPowerDbm >= 0 ) {
			std::string text = std::to_string(videostats->vtxPowerDbm) + "dBm";
			FontMeasureString( text, &w, &h );
			RenderText( (float)mBorderRight - w*0.775f, mBorderTop + mFontHeight * 6.0f, text, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.8f );
		}
		if ( videostats->vtxChannel > 0 ) {
			std::string text = "";
			if ( strlen(videostats->vtxBand) > 0 ) {
				if ( videostats->vtxBand[0] == 'B' ) {
					text += std::string(&videostats->vtxBand[7], 1);
				} else {
					text += std::string(&videostats->vtxBand[0], 1);
				}
			}
			text += std::to_string(videostats->vtxChannel % 8 + 1);
			FontMeasureString( text, &w, &h );
			RenderText( (float)mBorderRight - w*0.775f, mBorderTop + mFontHeight * 7.0f, text, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.8f );
		}
	}

	// Latency
	if ( dronestats ) {
		int w = 0, h = 0;
		float latency_red = std::min( 1.0f, ((float)dronestats->ping ) / 100.0f );
		std::string latency_str = std::to_string( dronestats->ping / 2 ) + "ms";
		FontMeasureString( latency_str, &w, &h );
		RenderText( (float)mBorderRight - w*0.775f, mBorderTop + mFontHeight * 2.0f, latency_str, Vector4f( 0.5f + 0.5f * latency_red, 1.0f - latency_red * 0.25f, 0.5f - latency_red * 0.5f, 1.0f ), 0.8f );
		if ( dronestats->ping > 50 ) {
			RenderText( mWidth * 0.5f, mBorderBottom - mHeight * 0.15f - mFontSize * 2, "High Latency", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, TextAlignment::CENTER );
		}
	}

	if ( dronestats and not std::isnan( dronestats->gpsSpeed ) ) {
		// Render dronestats->gpsSpeed as simple text, already in km/h
		std::string speed = std::to_string((int)dronestats->gpsSpeed) + "km/h";
		std::string sats = std::to_string((int)dronestats->gpsSatellitesUsed) + "/" + std::to_string((int)dronestats->gpsSatellitesSeen);
		RenderText( mBorderRight, mBorderBottom - 256 - 46, speed, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.75f, RendererHUD::END );
		RenderText( mBorderRight, mBorderBottom - 256 - 6, sats, Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.75f, RendererHUD::END );
	}

	int imsg = 0;
	for ( std::string msg : dronestats->messages ) {
		RenderText( mBorderLeft * 0.75f, mBorderTop + mFontSize * 0.75f * ( 3 + (imsg++) ), msg, Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 0.75f );
	}
}


void RendererHUDNeo::RenderThrustAcceleration( float thrust, float acceleration )
{
	if ( thrust < 0.0f ) {
		thrust = 0.0f;
	}
	if ( thrust > 1.0f ) {
		thrust = 1.0f;
	}
	if ( acceleration < 0.0f ) {
		acceleration = 0.0f;
	}
	if ( acceleration > 1.0f ) {
		acceleration = 1.0f;
	}
	glUseProgram( mShader.mShader );
	{
		if ( mNightMode ) {
			glUniform4f( mShader.mColorID, 0.2f, 0.8f, 0.7f, 1.0f );
		} else {
			glUniform4f( mShader.mColorID, 0.2f, 0.8f, 1.0f, 1.0f );
		}

		glBindBuffer( GL_ARRAY_BUFFER, mThrustVBO );
		glVertexAttribPointer( mShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
		glVertexAttribPointer( mShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

		glDisable( GL_CULL_FACE );

		DrawArrays( mShader, GL_TRIANGLE_STRIP, 0, 4 * std::min( 64, (int)( thrust * 64.0f ) ) );
	}
	{
		if ( mNightMode ) {
			glUniform4f( mShader.mColorID, 0.2f, 0.7f, 0.65f, 1.0f );
		} else {
			glUniform4f( mShader.mColorID, 0.3f, 0.75f, 0.7f, 1.0f );
		}

		glBindBuffer( GL_ARRAY_BUFFER, mAccelerationVBO );
		glVertexAttribPointer( mShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
		glVertexAttribPointer( mShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

		glDisable( GL_CULL_FACE );

		DrawArrays( mShader, GL_TRIANGLE_STRIP, 0, 4 * std::min( 32, (int)( acceleration * 32.0f ) ) );
	}
}


void RendererHUDNeo::RenderLink( float quality, float level )
{
	if ( quality > 1.0f ) {
		quality = 1.0f;
	}
	if ( quality < 0.0f ) {
		quality = 0.0f;
	}
	if ( level > 1.0f ) {
		level = 1.0f;
	}
	if ( level < 0.0f ) {
		level = 0.0f;
	}

	glUseProgram( mColorShader.mShader );
	glUniform4f( mColorShader.mColorID, 1.0f, 1.0f, 1.0f, 1.0f );
	glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
	glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
	glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
	glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

	FastVertexColor linkBuffer[6 * 16];
	memset( linkBuffer, 0, sizeof( linkBuffer ) );
	for ( uint32_t i = 0; i < 16; i++ ) {
		float height = std::exp( i * 0.1f );
		int ofsy = level * height * 12.0f;
		int bar_width = 11.0f * mFontSize / 30.0f;
		float x = mBorderLeft + i * ( bar_width + 2 );
		float y = mBorderTop + 60;

		linkBuffer[i*6 + 0].x = x;
		linkBuffer[i*6 + 0].y = y-ofsy;
		linkBuffer[i*6 + 1].x = x+bar_width;
		linkBuffer[i*6 + 1].y = y;
		linkBuffer[i*6 + 2].x = x+bar_width;
		linkBuffer[i*6 + 2].y = y-ofsy;
		linkBuffer[i*6 + 3].x = x;
		linkBuffer[i*6 + 3].y = y-ofsy;
		linkBuffer[i*6 + 4].x = x;
		linkBuffer[i*6 + 4].y = y;
		linkBuffer[i*6 + 5].x = x+bar_width;
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
	glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
	::Thread::EnterCritical();
	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(linkBuffer), linkBuffer );
	::Thread::ExitCritical();

	DrawArrays( mColorShader, GL_TRIANGLES, 0, 6 * std::min( 16, (int)( quality * 16.0f ) ) );
}


void RendererHUDNeo::RenderBattery( float level )
{
	if ( level < 0.0f ) {
		level = 0.0f;
	}
	if ( level > 1.0f ) {
		level = 1.0f;
	}
	glUseProgram( mShader.mShader );
	glUniform4f( mShader.mColorID, 1.0f - level * 0.5f, level, level * 0.5f, 1.0f );

// 	int ofsy = (int)( level * 96.0f );
	FastVertex batteryBuffer[4*8];
	Vector2f offset = Vector2f( mBorderLeft, mBorderBottom - 20 );
	for ( uint32_t i = 0; i < 8; i++ ) {
		Vector2f p0 = offset + Vector2f( 200.0f * level * (float)( i + 0 ) / 8.0f, 0.0f );
		Vector2f p1 = offset + Vector2f( 200.0f * level * (float)( i + 1 ) / 8.0f, 0.0f );
		Vector2f p2 = offset + Vector2f( 200.0f * level * (float)( i + 0 ) / 8.0f, 20.0f );
		Vector2f p3 = offset + Vector2f( 200.0f * level * (float)( i + 1 ) / 8.0f, 20.0f );

		batteryBuffer[i*4 + 0].x = p0.x;
		batteryBuffer[i*4 + 0].y = p0.y;
		batteryBuffer[i*4 + 1].x = p1.x;
		batteryBuffer[i*4 + 1].y = p1.y;
		batteryBuffer[i*4 + 2].x = p2.x;
		batteryBuffer[i*4 + 2].y = p2.y;
		batteryBuffer[i*4 + 3].x = p3.x;
		batteryBuffer[i*4 + 3].y = p3.y;

		for ( int j = 0; j < 4; j++ ) {
			Vector2f vec = VR_Distort( Vector2f( batteryBuffer[i*4 + j].x, batteryBuffer[i*4 + j].y ) );
			batteryBuffer[i*4 + j].x = vec.x;
			batteryBuffer[i*4 + j].y = vec.y;
		}
	}
	glBindBuffer( GL_ARRAY_BUFFER, mBatteryVBO );
	::Thread::EnterCritical();
	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(batteryBuffer), batteryBuffer );
	::Thread::ExitCritical();

	glVertexAttribPointer( mShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	DrawArrays( mShader, GL_TRIANGLE_STRIP, 0, sizeof(batteryBuffer)/sizeof(FastVertex) );
}


static Vector2f Rotate( const Vector2f& p0, float s, float c )
{
	Vector2f p;
	p.x =  p0.x * c - p0.y * s;
	p.y = p0.x * s + p0.y * c;
	return p;
}


void RendererHUDNeo::RenderAttitude( const Vector3f& rpy )
{
/*
	mSmoothRPY = mSmoothRPY + ( Vector3f( rpy.x, rpy.y, rpy.z ) - mSmoothRPY ) * 35.0f * ( 1.0f / 60.0f );
	if ( std::abs( mSmoothRPY.x ) <= 0.15f ) {
		mSmoothRPY.x = 0.0f;
	}
	if ( std::abs( mSmoothRPY.y ) <= 0.15f ) {
		mSmoothRPY.y = 0.0f;
	}
*/
	mSmoothRPY.x = rpy.x;
	mSmoothRPY.y = rpy.y;
	mSmoothRPY.z = rpy.z;

	glLineWidth( 2.0f );

	glBindBuffer( GL_ARRAY_BUFFER, mLineVBO );
	glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
	glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
	glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

	glUseProgram( mColorShader.mShader );
	uint32_t color = 0xFFFFFFFF;
	if ( mNightMode ) {
		color = 0xFF40FF40;
	}

	float xrot = -M_PI * ( mSmoothRPY.x / 180.0f );
	float yofs = -( mHeight / 2.0f ) * ( mSmoothRPY.y / 180.0f ) * M_PI * 2.25f;
	float s = std::sin( xrot );
	float c = std::cos( xrot );

	float movingBarWidth = 0.4f;
	float staticBarWidth = (1.0f - movingBarWidth ) / 2.0f;
	float movingBarOffset = staticBarWidth;

	static FastVertexColor allLinesBuffer[2*4*64 + 16];
	uint32_t total = 0;

	// Fixed lines
	{
		for ( uint32_t part = 0; part < 2; part++ ) {
			for ( uint32_t j = 0; j < 4; j++ ) {
				float xpos = ( part * ( staticBarWidth + movingBarWidth ) ) + staticBarWidth * (float)j / (float)4;
				float xpos1 = ( part * ( staticBarWidth + movingBarWidth ) ) + staticBarWidth * (float)(j + 1) / (float)4;
				allLinesBuffer[part * 2 + j * 4 + 0].x = xpos * mWidth;
				allLinesBuffer[part * 2 + j * 4 + 0].y = mHeight / 2.0f;
				allLinesBuffer[part * 2 + j * 4 + 0].color = color;
				allLinesBuffer[part * 2 + j * 4 + 1].x = xpos1 * mWidth;
				allLinesBuffer[part * 2 + j * 4 + 1].y = mHeight / 2.0f;
				allLinesBuffer[part * 2 + j * 4 + 1].color = color;
				total += 2;
			}
		}
	}

	if ( std::abs(mSmoothRPY.x) > 70.0f or std::abs(mSmoothRPY.y) > 70.0f ) {
		/*
		::Thread::EnterCritical();
		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(FastVertexColor) * total, allLinesBuffer );
		::Thread::ExitCritical();
		DrawArrays( mColorShader, GL_LINES, 0, total );
		 return;
		*/
	}

	// Attitude line
	{
		static FastVertexColor linesBuffer[8];
		for ( uint32_t j = 0; j < sizeof(linesBuffer)/sizeof(FastVertexColor); j++ ) {
			float xpos = movingBarOffset + movingBarWidth * (float)j / (float)(sizeof(linesBuffer)/sizeof(FastVertexColor)-1);
			Vector2f p;
			Vector2f p0 = Vector2f( ( xpos - 0.5f ) * ( mWidth ), yofs );
			p.x = mWidth / 2.0f + p0.x * c - p0.y * s;
			p.y = mHeight / 2.0f + p0.x * s + p0.y * c;
			p = VR_Distort( p );
// 			linesBuffer[j].x = std::min( std::max( p.x, 0.0f ), mWidth - 50.0f );
			linesBuffer[j].x = p.x;
			linesBuffer[j].y = p.y;
			linesBuffer[j].color = color;
		}

		::Thread::EnterCritical();
		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(linesBuffer), linesBuffer );
		::Thread::ExitCritical();

		DrawArrays( mColorShader, GL_LINE_STRIP, 0, sizeof(linesBuffer)/sizeof(FastVertexColor) );
	}

	// Degres lines
	{
		uint32_t i = total;
		for ( int32_t j = -3; j <= 3; j++ ) {
			if ( j == 0 ) {
				continue;
			}
			float y = 720.0f * 0.25f * ( (float)j / 2.0f );
			if ( std::abs( Rotate( Vector2f( 0.0f, yofs + y ), s, c ).y ) >= 720.0f * 0.5f * 1.25f ) {
				continue;
			}
			float text_line = 720.0f * 0.0075f;
			if ( j > 0 ) {
				text_line = -text_line;
			}
			Vector2f p0 = VR_Distort( Vector2f( mWidth * 0.5f, 720.0 * 0.5f ) + Rotate( Vector2f( mWidth * 0.5f * 0.15f, yofs + y ), s, c ) );
			Vector2f p1 = VR_Distort( Vector2f( mWidth * 0.5f, 720.0 * 0.5f ) + Rotate( Vector2f( mWidth * 0.5f * 0.225f, yofs + y ), s, c ) );
			Vector2f p2 = VR_Distort( Vector2f( mWidth * 0.5f, 720.0 * 0.5f ) + Rotate( Vector2f( mWidth * 0.5f * 0.225f, yofs + y + text_line ), s, c ) );
			allLinesBuffer[i].x = p0.x;
			allLinesBuffer[i].y = p0.y;
			allLinesBuffer[i++].color = color;
			allLinesBuffer[i].x = p1.x;
			allLinesBuffer[i].y = p1.y;
			allLinesBuffer[i++].color = color;
			allLinesBuffer[i].x = p1.x;
			allLinesBuffer[i].y = p1.y;
			allLinesBuffer[i++].color = color;
			allLinesBuffer[i].x = p2.x;
			allLinesBuffer[i].y = p2.y;
			allLinesBuffer[i++].color = color;
			Vector2f p3 = VR_Distort( Vector2f( mWidth * 0.5f, 720.0 * 0.5f ) + Rotate( Vector2f( mWidth * 0.5f * -0.15f, yofs + y ), s, c ) );
			Vector2f p4 = VR_Distort( Vector2f( mWidth * 0.5f, 720.0 * 0.5f ) + Rotate( Vector2f( mWidth * 0.5f * -0.225f, yofs + y ), s, c ) );
			Vector2f p5 = VR_Distort( Vector2f( mWidth * 0.5f, 720.0 * 0.5f ) + Rotate( Vector2f( mWidth * 0.5f * -0.225f, yofs + y + text_line ), s, c ) );
			allLinesBuffer[i].x = p3.x;
			allLinesBuffer[i].y = p3.y;
			allLinesBuffer[i++].color = color;
			allLinesBuffer[i].x = p4.x;
			allLinesBuffer[i].y = p4.y;
			allLinesBuffer[i++].color = color;
			allLinesBuffer[i].x = p4.x;
			allLinesBuffer[i].y = p4.y;
			allLinesBuffer[i++].color = color;
			allLinesBuffer[i].x = p5.x;
			allLinesBuffer[i].y = p5.y;
			allLinesBuffer[i++].color = color;
			total += 8;
		}
	}

	::Thread::EnterCritical();
	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(FastVertexColor) * total, allLinesBuffer );
	::Thread::ExitCritical();
	DrawArrays( mColorShader, GL_LINES, 0, total );
}


void RendererHUDNeo::Compute()
{
	float thrust_outer = mSpeedometerSize;
	float thrust_out = mSpeedometerSize * 0.77f;
	float thrust_in = mSpeedometerSize * 0.54f;
	{
		const uint32_t steps_count = 64;
		FastVertex thrustBuffer[4 * steps_count * 4];
		memset( thrustBuffer, 0, sizeof( thrustBuffer ) );
		Vector2f offset = Vector2f( mBorderRight - thrust_outer * 0.7f, mBorderBottom - thrust_outer * 0.75f );
		for ( uint32_t i = 0; i < steps_count; i++ ) {
			float angle0 = ( 0.1f + 0.775f * ( (float)i / (float)steps_count ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			float angle1 = ( 0.1f + 0.775f * ( (float)( i + 1 ) / (float)steps_count ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			Vector2f p0 = offset + thrust_in * Vector2f( std::cos( angle0 ), std::sin( angle0 ) );
			Vector2f p1 = offset + thrust_out * Vector2f( std::cos( angle0 ), std::sin( angle0 ) );
			Vector2f p2 = offset + thrust_in * Vector2f( std::cos( angle1 ), std::sin( angle1 ) );
			Vector2f p3 = offset + thrust_out * Vector2f( std::cos( angle1 ), std::sin( angle1 ) );

			thrustBuffer[i*4 + 0].x = p0.x;
			thrustBuffer[i*4 + 0].y = p0.y;
			thrustBuffer[i*4 + 1].x = p1.x;
			thrustBuffer[i*4 + 1].y = p1.y;
			thrustBuffer[i*4 + 2].x = p2.x;
			thrustBuffer[i*4 + 2].y = p2.y;
			thrustBuffer[i*4 + 3].x = p3.x;
			thrustBuffer[i*4 + 3].y = p3.y;

			for ( int j = 0; j < 4; j++ ) {
				Vector2f vec = VR_Distort( Vector2f( thrustBuffer[i*4 + j].x, thrustBuffer[i*4 + j].y ) );
				thrustBuffer[i*4 + j].x = vec.x;
				thrustBuffer[i*4 + j].y = vec.y;
			}
		}

		glGenBuffers( 1, &mThrustVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mThrustVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(thrustBuffer), thrustBuffer, GL_STATIC_DRAW );
	}

	{
		const uint32_t steps_count = 32;
		FastVertex accelerationBuffer[4 * steps_count * 8];
		memset( accelerationBuffer, 0, sizeof( accelerationBuffer ) );
		Vector2f offset = Vector2f( mBorderRight - thrust_outer * 0.7f, mBorderBottom - thrust_outer * 0.75f );
		for ( uint32_t i = 0; i < steps_count; i++ ) {
			float angle0 = ( 0.1f + 0.265f * ( (float)i / (float)steps_count ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			float angle1 = ( 0.1f + 0.265f * ( (float)( i + 1 ) / (float)steps_count ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			Vector2f p0 = offset + ( thrust_out + 3.0f ) * Vector2f( std::cos( angle0 ), std::sin( angle0 ) );
			Vector2f p1 = offset + ( thrust_outer - 4.0f ) * Vector2f( std::cos( angle0 ), std::sin( angle0 ) );
			Vector2f p2 = offset + ( thrust_out + 3.0f ) * Vector2f( std::cos( angle1 ), std::sin( angle1 ) );
			Vector2f p3 = offset + ( thrust_outer - 4.0f ) * Vector2f( std::cos( angle1 ), std::sin( angle1 ) );

			accelerationBuffer[i*4 + 0].x = p0.x;
			accelerationBuffer[i*4 + 0].y = p0.y;
			accelerationBuffer[i*4 + 1].x = p1.x;
			accelerationBuffer[i*4 + 1].y = p1.y;
			accelerationBuffer[i*4 + 2].x = p2.x;
			accelerationBuffer[i*4 + 2].y = p2.y;
			accelerationBuffer[i*4 + 3].x = p3.x;
			accelerationBuffer[i*4 + 3].y = p3.y;

			for ( int j = 0; j < 4; j++ ) {
				Vector2f vec = VR_Distort( Vector2f( accelerationBuffer[i*4 + j].x, accelerationBuffer[i*4 + j].y ) );
				accelerationBuffer[i*4 + j].x = vec.x;
				accelerationBuffer[i*4 + j].y = vec.y;
			}
		}
	
		glGenBuffers( 1, &mAccelerationVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mAccelerationVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(accelerationBuffer), accelerationBuffer, GL_STATIC_DRAW );
	}

	{
		FastVertexColor linkBuffer[6 * 16 * 4];
		memset( linkBuffer, 0, sizeof( linkBuffer ) );
		/*
		for ( uint32_t i = 0; i < 16; i++ ) {
			float height = std::exp( i * 0.1f );
			int ofsy = height * 12.0f;
			float x = 1280.0f / ( 1 + mStereo ) * 0.12f + i * ( 11 + 1 );
			float y = 720.0f * 0.22f;

			linkBuffer[i*6 + 0].x = x;
			linkBuffer[i*6 + 0].y = y-ofsy;
			linkBuffer[i*6 + 1].x = x+11;
			linkBuffer[i*6 + 1].y = y;
			linkBuffer[i*6 + 2].x = x+11;
			linkBuffer[i*6 + 2].y = y-ofsy;
			linkBuffer[i*6 + 3].x = x;
			linkBuffer[i*6 + 3].y = y-ofsy;
			linkBuffer[i*6 + 4].x = x;
			linkBuffer[i*6 + 4].y = y;
			linkBuffer[i*6 + 5].x = x+11;
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
		*/
		glGenBuffers( 1, &mLinkVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertexColor) * 6 * 16 * 4, linkBuffer, GL_STATIC_DRAW );
	}

	{
		glGenBuffers( 1, &mBatteryVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mBatteryVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 2048, nullptr, GL_DYNAMIC_DRAW );
	}

	{
		glGenBuffers( 1, &mLineVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mLineVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertexColor) * 2048, nullptr, GL_DYNAMIC_DRAW );
	}

	{
		FastVertexColor circleBuffer[2048];
		uint32_t steps_count = 64;
		for ( uint32_t i = 0; i < steps_count; i++ ) {
			float angle = ( 0.1f + 0.8f * ( (float)i / (float)steps_count ) ) * M_PI;
			Vector2f pos = Vector2f( 150.0f * std::cos( angle ), 150.0f * std::sin( angle ) );
			pos = VR_Distort( pos + Vector2f( mWidth / 2.0f, mHeight / 2.0f ) );
			circleBuffer[i].x = pos.x;
			circleBuffer[i].y = pos.y;
			circleBuffer[i].color = 0xFFFFFFFF;
			if ( i <= 10 ) {
				circleBuffer[i].color = ( ( i * 255 / 10 ) << 24 ) | 0x00FFFFFF;
			} else if ( i >= 54 ) {
				circleBuffer[i].color = ( ( ( steps_count - i ) * 255 / 10 ) << 24 ) | 0x00FFFFFF;
			}
			memcpy( &circleBuffer[ steps_count + i ], &circleBuffer[i], sizeof( circleBuffer[i] ) );
			circleBuffer[ steps_count + i ].y = 720.0f / 2.0f - ( pos.y - 720.0f / 2.0f );
		}

		uint32_t i = 128;
		steps_count = 32;
		for ( uint32_t j = 0; j < steps_count; j++ ) {
			float angle = ( 0.1f + 0.8f * ( (float)j / (float)steps_count ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			if ( j == 0 ) {
				Vector2f pos = VR_Distort( Vector2f( mBorderRight - thrust_outer * 0.7f + thrust_outer * std::cos( angle ), mBorderBottom - thrust_outer * 0.75f + thrust_outer * std::sin( angle ) ) );
				circleBuffer[i].x = pos.x;
				circleBuffer[i].y = pos.y;
				circleBuffer[i].color = 0xFFFFFFFF;
			}
			Vector2f pos = Vector2f( mBorderRight - thrust_outer * 0.7f + thrust_in * std::cos( angle ), mBorderBottom - thrust_outer * 0.75f + thrust_in * std::sin( angle ) );
			pos = VR_Distort( pos );
			circleBuffer[++i].x = pos.x;
			circleBuffer[i].y = pos.y;
			circleBuffer[i].color = 0xFFFFFFFF;
		}
		for ( uint32_t j = 0; j < steps_count; j++ ) {
			float angle = ( 0.1f + 0.8f * ( (float)( 31 - j ) / (float)steps_count ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			Vector2f pos = Vector2f( mBorderRight - thrust_outer * 0.7f + thrust_out * std::cos( angle ), mBorderBottom - thrust_outer * 0.75f + thrust_out * std::sin( angle ) );
			pos = VR_Distort( pos );
			circleBuffer[++i].x = pos.x;
			circleBuffer[i].y = pos.y;
			circleBuffer[i].color = 0xFFFFFFFF;
		}

		glGenBuffers( 1, &mCircleVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mCircleVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(circleBuffer), circleBuffer, GL_STATIC_DRAW );
	}

	{
		FastVertexColor staticLinesBuffer[2048];
		uint32_t i = 0;
		// G-force container
		{
			Vector2f p0 = Vector2f( mBorderRight - thrust_outer * 0.7f - thrust_outer * 0.3f, mBorderBottom - thrust_outer * 0.575f );
			Vector2f p1 = Vector2f( mBorderRight - thrust_outer * 0.7f + thrust_outer * 0.3f, mBorderBottom - thrust_outer * 0.575f + mFontSize * 0.55f );
			staticLinesBuffer[i].x = p0.x;
			staticLinesBuffer[i].y = p0.y;
			staticLinesBuffer[++i].x = p1.x;
			staticLinesBuffer[i].y = p0.y;

			staticLinesBuffer[++i].x = p1.x;
			staticLinesBuffer[i].y = p0.y;
			staticLinesBuffer[++i].x = p1.x;
			staticLinesBuffer[i].y = p1.y;

			staticLinesBuffer[++i].x = p0.x;
			staticLinesBuffer[i].y = p1.y;
			staticLinesBuffer[++i].x = p1.x;
			staticLinesBuffer[i].y = p1.y;

			staticLinesBuffer[++i].x = p0.x;
			staticLinesBuffer[i].y = p1.y;
			staticLinesBuffer[++i].x = p0.x;
			staticLinesBuffer[i].y = p0.y;
		}
		// Max G-circular-bar
		{
			Vector2f pos;
			float angle = M_PI * 1.25f;
			pos = Vector2f( mBorderRight - thrust_outer * 0.7f + thrust_out * std::cos( angle ), mBorderBottom - thrust_outer * 0.75f + thrust_out * std::sin( angle ) );
			staticLinesBuffer[++i].x = pos.x;
			staticLinesBuffer[i].y = pos.y;
			pos = Vector2f( mBorderRight - thrust_outer * 0.7f + thrust_outer * std::cos( angle ), mBorderBottom - thrust_outer * 0.75f + thrust_outer * std::sin( angle ) );
			staticLinesBuffer[++i].x = pos.x;
			staticLinesBuffer[i].y = pos.y;
		}
		// Battery container
		{
			Vector2f offset = Vector2f( mBorderLeft, mBorderBottom - 20 );
			for ( uint32_t j = 0; j < 8; j++ ) {
				Vector2f p0 = offset + Vector2f( 200.0f * (float)( j + 0 ) / 8.0f, 0.0f );
				Vector2f p1 = offset + Vector2f( 200.0f * (float)( j + 1 ) / 8.0f, 0.0f );
				staticLinesBuffer[++i].x = p0.x;
				staticLinesBuffer[i].y = p0.y;
				staticLinesBuffer[++i].x = p1.x;
				staticLinesBuffer[i].y = p1.y;
				staticLinesBuffer[++i].x = p0.x;
				staticLinesBuffer[i].y = p0.y + 20.0f;
				staticLinesBuffer[++i].x = p1.x;
				staticLinesBuffer[i].y = p1.y + 20.0f;
			}
			Vector2f p0 = offset + Vector2f( 200.0f, 0.0f );
			Vector2f p1 = offset + Vector2f( 200.0f, 20.0f );
			staticLinesBuffer[++i].x = p0.x;
			staticLinesBuffer[i].y = p0.y;
			staticLinesBuffer[++i].x = p1.x;
			staticLinesBuffer[i].y = p1.y;
			p0.x = p1.x = offset.x;
			staticLinesBuffer[++i].x = p0.x;
			staticLinesBuffer[i].y = p0.y;
			staticLinesBuffer[++i].x = p1.x;
			staticLinesBuffer[i].y = p1.y;
		}
		mStaticLinesCountNoAttitude = i + 1;

		// Left lines
		{
			for ( uint32_t j = 0; j <= 12; j++ ) {
				if ( j <= 11 ) {
					staticLinesBuffer[++i].x = 0;
					staticLinesBuffer[i].y = 180.0f + ( 720.0f - 360.0f ) * ( (float)j / 12.0f );
					staticLinesBuffer[++i].x = 0;
					staticLinesBuffer[i].y = 180.0f + ( 720.0f - 360.0f ) * ( (float)( j + 1 ) / 12.0f );
				}
				staticLinesBuffer[++i].x = 0;
				staticLinesBuffer[i].y = 180.0f + ( 720.0f - 360.0f ) * ( (float)j / 12.0f );
				staticLinesBuffer[++i].x = 1280.0f * 0.03f;
				staticLinesBuffer[i].y = 180.0f + ( 720.0f - 360.0f ) * ( (float)j / 12.0f );
			}
		}
/*
		// Right line
		{
			for ( uint32_t j = 0; j <= 6; j++ ) {
				staticLinesBuffer[++i].x = 1280.0f * 0.94f;
				staticLinesBuffer[i].y = 720.0f * 0.5f + 720.0f * 0.15f * ( (float)j / 6.0f - 0.5f );
				staticLinesBuffer[++i].x = 1280.0f * 0.94f;
				staticLinesBuffer[i].y = 720.0f * 0.5f + 720.0f * 0.15f * ( (float)( j + 1 ) / 6.0f - 0.5f );
			}
		}
*/
		mStaticLinesCount = i + 1;
		for ( uint32_t i = 0; i < mStaticLinesCount; i++ ) {
			Vector2f pos = VR_Distort( Vector2f( staticLinesBuffer[i].x, staticLinesBuffer[i].y ) );
			staticLinesBuffer[i].x = pos.x;
			staticLinesBuffer[i].y = pos.y;
			staticLinesBuffer[i].color = 0xFFFFFFFF;
		}

		glGenBuffers( 1, &mStaticLinesVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mStaticLinesVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(staticLinesBuffer), staticLinesBuffer, GL_STATIC_DRAW );
	}
}
