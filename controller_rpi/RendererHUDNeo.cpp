#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cmath>
#include <sstream>
#include <gammaengine/File.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include "RendererHUDNeo.h"
#include "Controller.h"


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

	uniform vec4 color;
	in vec4 ge_Color;

	void main()
	{
		ge_FragColor = color * ge_Color;
	})"
;


RendererHUDNeo::RendererHUDNeo( Instance* instance, Font* font )
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


RendererHUDNeo::~RendererHUDNeo()
{
}


void RendererHUDNeo::Render( GE::Window* window, Controller* controller, VideoStats* videostats, IwStats* iwstats )
{
	// Controls
	{
		float thrust = controller->thrust();
		if ( not controller->armed() ) {
			thrust = 0.0f;
		}
		float acceleration = controller->acceleration() / 9.7f;
		RenderAttitude( controller->rpy() );
		RenderThrustAcceleration( thrust, std::min( 1.0f, acceleration / 10.0f ) );

		Controller::Mode mode = controller->mode();
		Vector4f color_acro = Vector4f( 0.4f, 0.4f, 0.4f, 1.0f );
		Vector4f color_hrzn = Vector4f( 0.4f, 0.4f, 0.4f, 1.0f );
		if ( mode == Controller::Rate ) {
			color_acro.x = color_acro.y = color_acro.z = 1.0f;
		} else if ( mode == Controller::Stabilize ) {
			color_hrzn.x = color_hrzn.y = color_hrzn.z = 1.0f;
		}
		int w = 0, h = 0;
		mFont->measureString( "ACRO/HRZN", &w, &h );
		RenderText( (float)window->width() * ( 1.0f / 4.0f + 0.6f * 1.0f / 4.0f ) - w*0.45f - 8, window->height() - 130, "ACRO", color_acro, 0.45f );
		mFont->measureString( "/HRZN", &w, &h );
		RenderText( (float)window->width() * ( 1.0f / 4.0f + 0.6f * 1.0f / 4.0f ) - w*0.45f - 8, window->height() - 130, "/", Vector4f( 0.65f, 0.65f, 0.65f, 1.0f ), 0.45f );
		mFont->measureString( "HRZN", &w, &h );
		RenderText( (float)window->width() * ( 1.0f / 4.0f + 0.6f * 1.0f / 4.0f ) - w*0.45f - 8, window->height() - 130, "HRZN", color_hrzn, 0.45f );

		Vector4f color = Vector4f( 1.0f, 1.0f, 1.0f, 1.0f );
		std::string saccel = std::to_string( acceleration );
		saccel = saccel.substr( 0, saccel.find( "." ) + 2 );
		RenderText( (float)window->width() * ( 1.0f / 4.0f + 0.4665f * 1.0f / 4.0f ), window->height() - 145, saccel + "g", color, 0.55f, true );
		if ( controller->armed() ) {
			RenderText( (float)window->width() * ( 1.0f / 4.0f + 0.47f * 1.0f / 4.0f ), window->height() - 165, std::to_string( (int)( thrust * 100.0f ) ) + "%", color, 1.0f, true );
		} else {
			RenderText( (float)window->width() * ( 1.0f / 4.0f + 0.47f * 1.0f / 4.0f ), window->height() - 168, "disarmed", color, 0.6f, true );
		}
	}

	// Battery
	{
		float level = controller->batteryLevel();
		RenderBattery( level );
		if ( level <= 0.25f and mBlinkingViews ) {
			RenderText( (float)window->width() * 1.0f / 4.0f, window->height() - 125, "Low Battery", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, true );
		}
		float battery_red = 1.0f - level;
		RenderText( (float)window->width() * 0.5f * 0.30f, window->height() * 0.79f, std::to_string( (int)( level * 100.0f ) ) + "%", Vector4f( 0.5f + 0.5f * battery_red, 1.0f - battery_red * 0.25f, 0.5f - battery_red * 0.5f, 1.0f ), 0.9 );
		std::string svolt = std::to_string( controller->batteryVoltage() );
		svolt = svolt.substr( 0, svolt.find( "." ) + 2 );
		RenderText( (float)window->width() * 0.5f * 0.12f, window->height() * 0.71f, svolt + "V", Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.75 );
		RenderText( (float)window->width() * 0.5f * 0.13f, window->height() * 0.75f, std::to_string( controller->totalCurrent() ) + "mAh", Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ), 0.9 );

		float localVoltage = controller->localBatteryVoltage();
		if ( localVoltage <= 3.2f * 2.0f and mBlinkingViews ) {
			RenderText( (float)window->width() * 1.0f / 4.0f, window->height() - 155, "Low Controller Battery", Vector4f( 1.0f, 0.5f, 0.5f, 1.0f ), 1.0f, true );
		}
		battery_red = 1.0f - ( ( localVoltage - 6.4f ) / 2.0f );
		svolt = std::to_string( localVoltage );
		svolt = svolt.substr( 0, svolt.find( "." ) + 3 );
		RenderText( (float)window->width() * 0.5f * 0.12f, window->height() * 0.67f, svolt + "V", Vector4f( 0.5f + 0.5f * battery_red, 1.0f - battery_red * 0.25f, 0.5f - battery_red * 0.5f, 1.0f ), 0.75 );
	}

	// Static elements
	{
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

			glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
			glDrawArrays( GL_LINES, 0, mStaticLinesCount );
			glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
			glDrawArrays( GL_LINES, 0, mStaticLinesCount );
		}
		// Circle
		{
			glLineWidth( 2.0f );

			glBindBuffer( GL_ARRAY_BUFFER, mCircleVBO );
			glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
			glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
			glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

			glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
			glDrawArrays( GL_LINE_STRIP, 0, 64 );
			glDrawArrays( GL_LINE_STRIP, 64, 64 );
			glDrawArrays( GL_LINE_STRIP, 128, 65 );
			glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
			glDrawArrays( GL_LINE_STRIP, 0, 64 );
			glDrawArrays( GL_LINE_STRIP, 64, 64 );
			glDrawArrays( GL_LINE_STRIP, 128, 65 );
		}
	}

	// Link status
	{
		RenderLink( (float)iwstats->qual / 100.0f );
		float link_red = 1.0f - ((float)iwstats->qual) / 100.0f;
		std::string link_quality_str = "Ch" + std::to_string( iwstats->channel ) + "  " + std::to_string( iwstats->level ) + "dBm  " + std::to_string( iwstats->qual ) + "%";
		RenderText( (float)window->width() * 0.5f * 0.13f, 720.0f * 0.21f, link_quality_str, Vector4f( 0.5f + 0.5f * link_red, 1.0f - link_red * 0.25f, 0.5f - link_red * 0.5f, 1.0f ), 0.9f );
	}

	// FPS + latency
	{
		int w = 0, h = 0;
		float fps_red = std::max( 0.0f, std::min( 1.0f, ((float)( 50 - videostats->fps ) ) / 50.0f ) );
		std::string fps_str = std::to_string( videostats->fps ) + "fps";
		mFont->measureString( fps_str, &w, &h );
		RenderText( (float)window->width() * ( 1.0f / 2.0f - 0.075f ) - w*0.8f, 120, fps_str, Vector4f( 0.5f + 0.5f * fps_red, 1.0f - fps_red * 0.25f, 0.5f - fps_red * 0.5f, 1.0f ), 0.8f );

		float latency_red = std::min( 1.0f, ((float)controller->ping()) / 50.0f );
		std::string latency_str = std::to_string( controller->ping() ) + "ms";
		mFont->measureString( latency_str, &w, &h );
		RenderText( (float)window->width() * ( 1.0f / 2.0f - 0.075f ) - w, 140, latency_str, Vector4f( 0.5f + 0.5f * latency_red, 1.0f - latency_red * 0.25f, 0.5f - latency_red * 0.5f, 1.0f ) );
	}
}


void RendererHUDNeo::RenderThrustAcceleration( float thrust, float acceleration )
{
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

		glUniform2f( mShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 * std::min( 64, (int)( thrust * 64.0f ) ) );
		glUniform2f( mShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 * std::min( 64, (int)( thrust * 64.0f ) ) );
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

		glUniform2f( mShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 * std::min( 32, (int)( acceleration * 32.0f ) ) );
		glUniform2f( mShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 * std::min( 32, (int)( acceleration * 32.0f ) ) );
	}
}


void RendererHUDNeo::RenderLink( float quality )
{
	glUseProgram( mColorShader.mShader );
	glUniform4f( mColorShader.mColorID, 1.0f, 1.0f, 1.0f, 1.0f );
	glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
	glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
	glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
	glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

	glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( quality * 16.0f ) ) );
	glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( quality * 16.0f ) ) );
}


void RendererHUDNeo::RenderBattery( float level )
{
	glUseProgram( mShader.mShader );
	glUniform4f( mShader.mColorID, 1.0f - level * 0.5f, level, level * 0.5f, 1.0f );

	int ofsy = (int)( level * 96.0f );
	FastVertex batteryBuffer[4*8];
	Vector2f offset = Vector2f( 1280.0f * 0.5f * 0.14f, 720.0f * 0.8f );
	for ( uint32_t i = 0; i < 8; i++ ) {
		Vector2f p0 = offset + Vector2f( 1280.0f * 0.5f * 0.15f * level * (float)( i + 0 ) / 8.0f, 0.0f );
		Vector2f p1 = offset + Vector2f( 1280.0f * 0.5f * 0.15f * level * (float)( i + 1 ) / 8.0f, 0.0f );
		Vector2f p2 = offset + Vector2f( 1280.0f * 0.5f * 0.15f * level * (float)( i + 0 ) / 8.0f, 720.0f * 0.028f );
		Vector2f p3 = offset + Vector2f( 1280.0f * 0.5f * 0.15f * level * (float)( i + 1 ) / 8.0f, 720.0f * 0.028f );

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
	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(batteryBuffer), batteryBuffer );

	glVertexAttribPointer( mShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	glUniform2f( mShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, sizeof(batteryBuffer)/sizeof(FastVertex) );
	glUniform2f( mShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, sizeof(batteryBuffer)/sizeof(FastVertex) );
}


static Vector2f Rotate( const Vector2f& p0, float s, float c )
{
	Vector2f p;
	p.x =  p0.x * c - p0.y * s;
	p.y = p0.x * s + p0.y * c;
	return p;
}


void RendererHUDNeo::RenderAttitude( const vec3& rpy )
{
	mSmoothRPY = mSmoothRPY + ( Vector3f( rpy.x, rpy.y, rpy.z ) - mSmoothRPY ) * 35.0f * ( 1.0f / 60.0f );
	if ( std::abs( mSmoothRPY.x ) <= 0.15f ) {
		mSmoothRPY.x = 0.0f;
	}
	if ( std::abs( mSmoothRPY.y ) <= 0.15f ) {
		mSmoothRPY.y = 0.0f;
	}

	glLineWidth( 1.0f );

	glBindBuffer( GL_ARRAY_BUFFER, mLineVBO );
	glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
	glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
	glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

	glUseProgram( mColorShader.mShader );
	uint32_t color = 0xFFFFFFFF;
	if ( mNightMode ) {
		color = 0xFF40FF40;
	}

	float xrot = M_PI * ( -mSmoothRPY.x / 180.0f );
	float yofs = ( 720.0f / 2.0f ) * ( -mSmoothRPY.y / 180.0f ) * M_PI * 2.75f;
	float s = std::sin( xrot );
	float c = std::cos( xrot );

	// Attitude line
	{
		FastVertexColor linesBuffer[16];
		for ( uint32_t j = 0; j < sizeof(linesBuffer)/sizeof(FastVertexColor); j++ ) {
			float xpos = (float)j / (float)(sizeof(linesBuffer)/sizeof(FastVertexColor));
			Vector2f p;
			Vector2f p0 = Vector2f( ( xpos - 0.5f ) * ( 1280.0f / 2.0f ), yofs );
			p.x = 1280.0f / 4.0f + p0.x * c - p0.y * s;
			p.y = 720.0f / 2.0f + p0.x * s + p0.y * c;
			p = VR_Distort( p );
			linesBuffer[j].x = std::min( std::max( p.x, 0.0f ), 1280.0f * 0.5f - 50.0f );
			linesBuffer[j].y = p.y;
			linesBuffer[j].color = color;
		}

		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(linesBuffer), linesBuffer );
		glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
		glDrawArrays( GL_LINE_STRIP, 0, sizeof(linesBuffer)/sizeof(FastVertexColor) );
		glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
		glDrawArrays( GL_LINE_STRIP, 0, sizeof(linesBuffer)/sizeof(FastVertexColor) );
	}

	// Degres lines
	{
		FastVertexColor linesBuffer[2*4*64];
		uint32_t i = 0;
		for ( int32_t j = -5; j <= 5; j++ ) {
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
			Vector2f p0 = VR_Distort( Vector2f( 1280.0f * 0.25f, 720.0 * 0.5f ) + Rotate( Vector2f( 1280.0f * 0.5f * 0.15f, yofs + y ), s, c ) );
			Vector2f p1 = VR_Distort( Vector2f( 1280.0f * 0.25f, 720.0 * 0.5f ) + Rotate( Vector2f( 1280.0f * 0.5f * 0.225f, yofs + y ), s, c ) );
			Vector2f p2 = VR_Distort( Vector2f( 1280.0f * 0.25f, 720.0 * 0.5f ) + Rotate( Vector2f( 1280.0f * 0.5f * 0.225f, yofs + y + text_line ), s, c ) );
			linesBuffer[i].x = p0.x;
			linesBuffer[i].y = p0.y;
			linesBuffer[i++].color = color;
			linesBuffer[i].x = p1.x;
			linesBuffer[i].y = p1.y;
			linesBuffer[i++].color = color;
			linesBuffer[i].x = p1.x;
			linesBuffer[i].y = p1.y;
			linesBuffer[i++].color = color;
			linesBuffer[i].x = p2.x;
			linesBuffer[i].y = p2.y;
			linesBuffer[i++].color = color;
			Vector2f p3 = VR_Distort( Vector2f( 1280.0f * 0.25f, 720.0 * 0.5f ) + Rotate( Vector2f( 1280.0f * 0.5f * -0.15f, yofs + y ), s, c ) );
			Vector2f p4 = VR_Distort( Vector2f( 1280.0f * 0.25f, 720.0 * 0.5f ) + Rotate( Vector2f( 1280.0f * 0.5f * -0.225f, yofs + y ), s, c ) );
			Vector2f p5 = VR_Distort( Vector2f( 1280.0f * 0.25f, 720.0 * 0.5f ) + Rotate( Vector2f( 1280.0f * 0.5f * -0.225f, yofs + y + text_line ), s, c ) );
			linesBuffer[i].x = p3.x;
			linesBuffer[i].y = p3.y;
			linesBuffer[i++].color = color;
			linesBuffer[i].x = p4.x;
			linesBuffer[i].y = p4.y;
			linesBuffer[i++].color = color;
			linesBuffer[i].x = p4.x;
			linesBuffer[i].y = p4.y;
			linesBuffer[i++].color = color;
			linesBuffer[i].x = p5.x;
			linesBuffer[i].y = p5.y;
			linesBuffer[i++].color = color;
		}

		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(FastVertexColor)*i, linesBuffer );

		glUniform2f( mColorShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
		glDrawArrays( GL_LINES, 0, i );
		glUniform2f( mColorShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
		glDrawArrays( GL_LINES, 0, i );
	}
}


void RendererHUDNeo::Compute()
{
// 	float thrust_outer = 80.0f;
// 	float thrust_out = 65.0f;
// // 	float thrust_in = 50.0f;
	float thrust_outer = 65.0f;
	float thrust_out = 50.0f;
	float thrust_in = 35.0f;
	{
		const uint32_t steps_count = 64;
		FastVertex thrustBuffer[4 * steps_count];
		memset( thrustBuffer, 0, sizeof( thrustBuffer ) );
		Vector2f offset = Vector2f( 640.0f - 170.0f, 720.0f - 145.0f );
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
		FastVertex accelerationBuffer[4 * steps_count];
		memset( accelerationBuffer, 0, sizeof( accelerationBuffer ) );
		Vector2f offset = Vector2f( 640.0f - 170.0f, 720.0f - 145.0f );
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
		FastVertexColor linkBuffer[6 * 16];
		memset( linkBuffer, 0, sizeof( linkBuffer ) );
		for ( uint32_t i = 0; i < 16; i++ ) {
			float height = std::exp( i * 0.1f );
			int ofsy = height * 12.0f;
			float x = 1280.0f * 0.5f * 0.12f + i * ( 11 + 1 );
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
		glGenBuffers( 1, &mLinkVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertexColor) * 6 * 16, linkBuffer, GL_STATIC_DRAW );
	}

	{
		glGenBuffers( 1, &mBatteryVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mBatteryVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 64, nullptr, GL_DYNAMIC_DRAW );
	}

	{
		glGenBuffers( 1, &mLineVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mLineVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertexColor) * 2 * 128, nullptr, GL_DYNAMIC_DRAW );
	}

	{
		FastVertexColor circleBuffer[512];
		for ( uint32_t i = 0; i < 64; i++ ) {
			float angle = ( 0.1f + 0.8f * ( (float)i / 64.0f ) ) * M_PI;
			Vector2f pos = Vector2f( 150.0f * std::cos( angle ), 150.0f * std::sin( angle ) );
			pos = VR_Distort( pos + Vector2f( 1280.0f / 4.0f, 720.0f / 2.0f ) );
			circleBuffer[i].x = pos.x;
			circleBuffer[i].y = pos.y;
			circleBuffer[i].color = 0xFFFFFFFF;
			if ( i <= 10 ) {
				circleBuffer[i].color = ( ( i * 255 / 10 ) << 24 ) | 0x00FFFFFF;
			} else if ( i >= 54 ) {
				circleBuffer[i].color = ( ( ( 64 - i ) * 255 / 10 ) << 24 ) | 0x00FFFFFF;
			}
			memcpy( &circleBuffer[ 64 + i ], &circleBuffer[i], sizeof( circleBuffer[i] ) );
			circleBuffer[ 64 + i ].y = 720.0f / 2.0f - ( pos.y - 720.0f / 2.0f );
		}

		uint32_t i = 128;
		for ( uint32_t j = 0; j < 32; j++ ) {
			float angle = ( 0.1f + 0.8f * ( (float)j / 32.0f ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			if ( j == 0 ) {
				Vector2f pos = VR_Distort( Vector2f( ( 640.0f - 170.0f ) + thrust_outer * std::cos( angle ), ( 720.0f - 145.0f ) + thrust_outer * std::sin( angle ) ) );
				circleBuffer[i].x = pos.x;
				circleBuffer[i].y = pos.y;
				circleBuffer[i].color = 0xFFFFFFFF;
			}
			Vector2f pos = Vector2f( ( 640.0f - 170.0f ) + thrust_in * std::cos( angle ), ( 720.0f - 145.0f ) + thrust_in * std::sin( angle ) );
			pos = VR_Distort( pos );
			circleBuffer[++i].x = pos.x;
			circleBuffer[i].y = pos.y;
			circleBuffer[i].color = 0xFFFFFFFF;
		}
		for ( uint32_t j = 0; j < 32; j++ ) {
			float angle = ( 0.1f + 0.8f * ( (float)( 31 - j ) / 32.0f ) ) * M_PI * 2.0f - ( M_PI * 1.48f );
			Vector2f pos = Vector2f( ( 640.0f - 170.0f ) + thrust_out * std::cos( angle ), ( 720.0f - 145.0f ) + thrust_out * std::sin( angle ) );
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
		FastVertexColor staticLinesBuffer[512];
		uint32_t i = 0;
		// G-force container
		{
			Vector2f p0 = Vector2f( ( 1280.0f / 2.0f ) * 0.705f, 720.0f - 113.0f );
			Vector2f p1 = Vector2f( ( 1280.0f / 2.0f ) * 0.765f, 720.0f - 130.0f );
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
			pos = Vector2f( ( 640.0f - 170.0f ) + thrust_out * std::cos( angle ), ( 720.0f - 145.0f ) + thrust_out * std::sin( angle ) );
			staticLinesBuffer[++i].x = pos.x;
			staticLinesBuffer[i].y = pos.y;
			pos = Vector2f( ( 640.0f - 170.0f ) + thrust_outer * std::cos( angle ), ( 720.0f - 145.0f ) + thrust_outer * std::sin( angle ) );
			staticLinesBuffer[++i].x = pos.x;
			staticLinesBuffer[i].y = pos.y;
		}
		// Battery container
		{
			Vector2f offset = Vector2f( 1280.0f * 0.5f * 0.14f, 720.0f * 0.8f );
			for ( uint32_t j = 0; j < 8; j++ ) {
				Vector2f p0 = offset + Vector2f( 1280.0f * 0.5f * 0.15f * (float)( j + 0 ) / 8.0f, 0.0f );
				Vector2f p1 = offset + Vector2f( 1280.0f * 0.5f * 0.15f * (float)( j + 1 ) / 8.0f, 0.0f );
				staticLinesBuffer[++i].x = p0.x;
				staticLinesBuffer[i].y = p0.y;
				staticLinesBuffer[++i].x = p1.x;
				staticLinesBuffer[i].y = p1.y;
				staticLinesBuffer[++i].x = p0.x;
				staticLinesBuffer[i].y = p0.y + 720.0f * 0.028f;
				staticLinesBuffer[++i].x = p1.x;
				staticLinesBuffer[i].y = p1.y + 720.0f * 0.028f;
			}
			Vector2f p0 = offset + Vector2f( 1280.0f * 0.5f * 0.15f, 0.0f );
			Vector2f p1 = offset + Vector2f( 1280.0f * 0.5f * 0.15f, 720.0f * 0.028f );
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
				staticLinesBuffer[++i].x = ( 1280.0f / 2.0f ) * 0.03f;
				staticLinesBuffer[i].y = 180.0f + ( 720.0f - 360.0f ) * ( (float)j / 12.0f );
			}
		}
		// Right line
		{
			for ( uint32_t j = 0; j <= 6; j++ ) {
				staticLinesBuffer[++i].x = 1280.0f * 0.5f * 0.94f;
				staticLinesBuffer[i].y = 720.0f * 0.5f + 720.0f * 0.15f * ( (float)j / 6.0f - 0.5f );
				staticLinesBuffer[++i].x = 1280.0f * 0.5f * 0.94f;
				staticLinesBuffer[i].y = 720.0f * 0.5f + 720.0f * 0.15f * ( (float)( j + 1 ) / 6.0f - 0.5f );
			}
		}

		mStaticLinesCount = i;
		for ( uint32_t i = 0; i < sizeof(staticLinesBuffer)/sizeof(FastVertexColor); i++ ) {
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
