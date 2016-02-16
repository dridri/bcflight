#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gammaengine/File.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include "RendererHUD.h"


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


static const char hud_text_vertices_shader[] =
R"(	#define in attribute
	#define out varying
	#define ge_Position gl_Position
	in vec2 ge_VertexTexcoord;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2 offset;
	out vec2 ge_TextureCoord;

	vec2 VR_Distort( vec2 coords )
	{
		vec2 ret = coords;
		vec2 ofs = vec2(1280.0/4.0, 720.0/2.0);
		if ( coords.x >= 1280.0/2.0 ) {
			ofs.x = 3.0 * 1280.0/4.0;
		}
		vec2 offset = coords - ofs;
		float r2 = dot( offset.xy, offset.xy );
		float r = sqrt(r2);
		float k1 = 1.95304;
		k1 = k1 / ((1280.0 / 4.0)*(1280.0 / 4.0) * 16.0);

		ret = ofs + ( 1.0 - k1 * r * r ) * offset;

		return ret;
	}


	void main()
	{
		ge_TextureCoord = ge_VertexTexcoord.xy;
		vec2 pos = VR_Distort( offset + ge_VertexPosition.xy );
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
	})"
;

static const char hud_text_fragment_shader[] =
R"(	#define in varying
	#define ge_FragColor gl_FragColor
	uniform sampler2D ge_Texture0;

	uniform vec4 color;
	in vec2 ge_TextureCoord;

	void main()
	{
		ge_FragColor = color * texture2D( ge_Texture0, ge_TextureCoord.xy );
	})"
;


Vector2f VR_Distort( const Vector2f& coords )
{
	Vector2f ret = coords;
	Vector2f ofs = Vector2f( 1280.0 / 4.0, 720.0 / 2.0 );
	if ( coords.x >= 1280.0 / 2.0 ) {
		ofs.x = 3.0 * 1280.0 / 4.0;
	}
	Vector2f offset = coords - ofs;
	float r2 = offset.x * offset.x + offset.y * offset.y;
	float r = std::sqrt( r2 );
	float k1 = 1.95304;
	k1 = k1 / ( ( 1280.0 / 4.0 ) * ( 1280.0 / 4.0 ) * 16.0 );

	ret = ofs + ( 1.0f - k1 * r * r ) * offset;

	return ret;
}


RendererHUD::RendererHUD( Instance* instance, Font* font )
	: mInstance( instance )
	, mFont( font )
	, mReady( false )
	, mMatrixProjection( new Matrix() )
	, mShader{ 0 }
	, mColorShader{ 0 }
	, mTextShader{ 0 }
{
	mMatrixProjection->Orthogonal( 0.0, 1280, 720, 0.0, -2049.0, 2049.0 );

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

	LoadVertexShader( &mTextShader, hud_text_vertices_shader, sizeof(hud_text_vertices_shader) + 1 );
	LoadFragmentShader( &mTextShader, hud_text_fragment_shader, sizeof(hud_text_fragment_shader) + 1 );
	createPipeline( &mTextShader );
	glUseProgram( mTextShader.mShader );
	glEnableVertexAttribArray( mTextShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mTextShader.mVertexPositionID );
	glUseProgram( 0 );


	Compute();
	glDisable( GL_DEPTH_TEST );
}


RendererHUD::~RendererHUD()
{
}


void RendererHUD::RenderThrust( float thrust )
{
	glUseProgram( mShader.mShader );
	glUniform4f( mShader.mColorID, 0.65f, 1.0f, 0.75f, 1.0f );

	glBindBuffer( GL_ARRAY_BUFFER, mThrustVBO );
	glVertexAttribPointer( mShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	glUniform2f( mShader.mOffsetID, 1280.0f * +0.005f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( thrust * 16.0f ) ) );
	glUniform2f( mShader.mOffsetID, 1280.0f * -0.005f + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( thrust * 16.0f ) ) );
}


void RendererHUD::RenderLink( float quality )
{
	glUseProgram( mColorShader.mShader );
// 	glUniform4f( mColorShader.mColorID, 0.65f, 1.0f, 0.75f, 1.0f );
	glBindBuffer( GL_ARRAY_BUFFER, mLinkVBO );
	glVertexAttribPointer( mColorShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( 0 ) );
	glVertexAttribPointer( mColorShader.mVertexColorID, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 ) );
	glVertexAttribPointer( mColorShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertexColor ), (void*)( sizeof( float ) * 2 + sizeof( uint32_t ) ) );

	glUniform2f( mColorShader.mOffsetID, 1280.0f * +0.005f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( quality * 16.0f ) ) );
	glUniform2f( mColorShader.mOffsetID, 1280.0f * -0.005f + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 * std::min( 16, (int)( quality * 16.0f ) ) );
}


void RendererHUD::RenderBattery( float level )
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

	glUniform2f( mShader.mOffsetID, 1280.0f * +0.005f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	glUniform2f( mShader.mOffsetID, 1280.0f * -0.005f + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
}


void RendererHUD::RenderText( int x, int y, const std::string& text, uint32_t color, bool hcenter )
{
	RenderText( x, y, text, Vector4f( (float)( color & 0xFF ) / 256.0f, (float)( ( color >> 8 ) & 0xFF ) / 256.0f, (float)( ( color >> 16 ) & 0xFF ) / 256.0f, 1.0f ), hcenter );
}


void RendererHUD::RenderText( int x, int y, const std::string& text, const Vector4f& color, bool hcenter )
{
	y += mFont->size();

	glUseProgram( mTextShader.mShader );
	glUniform4f( mTextShader.mColorID, color.x, color.y, color.z, color.w );
	glBindTexture( GL_TEXTURE_2D, mFont->texture()->serverReference( mInstance ) );

	glBindBuffer( GL_ARRAY_BUFFER, mTextVBO );
	glVertexAttribPointer( mTextShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mTextShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	if ( hcenter ) {
		int w = 0;
		for ( uint32_t i = 0; i < text.length(); i++ ) {
			w += mTextAdv[ (uint8_t)( (int)text.data()[i] ) ];
		}
		x -= w / 2;
	}

	for ( uint32_t i = 0; i < text.length(); i++ ) {
		uint8_t c = (uint8_t)( (int)text.data()[i] );
		glUniform2f( mTextShader.mOffsetID, 1280.0f * +0.005f + x, y );
		glDrawArrays( GL_TRIANGLES, c * 6, 6 );
		glUniform2f( mTextShader.mOffsetID, 1280.0f * -0.005f + 1280.0f / 2.0f + x, y );
		glDrawArrays( GL_TRIANGLES, c * 6, 6 );
		x += mTextAdv[ c ];
	}
}


void RendererHUD::Compute()
{
	{
		FastVertex thrustBuffer[6 * 16];
		memset( thrustBuffer, 0, sizeof( thrustBuffer ) );
		for ( uint32_t i = 0; i < 16; i++ ) {
			float height = std::exp( i * 0.1f );
			int ofsy = height * 10.0f;
			float x = 50 + i * ( 10 + 1 );
			float y = 720 - 75 - ofsy;
			
			thrustBuffer[i*6 + 0].x = x;
			thrustBuffer[i*6 + 0].y = y;
			thrustBuffer[i*6 + 1].x = x+10;
			thrustBuffer[i*6 + 1].y = y+ofsy;
			thrustBuffer[i*6 + 2].x = x+10;
			thrustBuffer[i*6 + 2].y = y;
			thrustBuffer[i*6 + 3].x = x;
			thrustBuffer[i*6 + 3].y = y;
			thrustBuffer[i*6 + 4].x = x;
			thrustBuffer[i*6 + 4].y = y+ofsy;
			thrustBuffer[i*6 + 5].x = x+10;
			thrustBuffer[i*6 + 5].y = y+ofsy;

			for ( int j = 0; j < 6; j++ ) {
				Vector2f vec = VR_Distort( Vector2f( thrustBuffer[i*6 + j].x, thrustBuffer[i*6 + j].y ) );
				thrustBuffer[i*6 + j].x = vec.x;
				thrustBuffer[i*6 + j].y = vec.y;
			}
		}
		glGenBuffers( 1, &mThrustVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mThrustVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 6 * 16, thrustBuffer, GL_STATIC_DRAW );
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
		mFont->texture()->serverReference( mInstance );
		const float rx = 1.0f / mFont->texture()->meta( "gles:width", (float)mFont->texture()->width() );
		const float ry = 1.0f / mFont->texture()->meta( "gles:height", (float)mFont->texture()->height() );
		FastVertex charactersBuffer[6 * 256];
		memset( charactersBuffer, 0, sizeof( charactersBuffer ) );
		for ( uint32_t i = 0; i < 256; i++ ) {
			uint8_t c = i;
			float sx = ( (float)mFont->glyph(c)->x ) * rx;
			float sy = ( (float)mFont->glyph(c)->y ) * ry;
			float texMaxX = ( (float)mFont->glyph(c)->w ) * rx;
			float texMaxY = ( (float)mFont->glyph(c)->h ) * ry;
			float width = mFont->glyph(c)->w;
			float height = mFont->glyph(c)->h;
			float fy = (float)0.0f - mFont->glyph(c)->posY;
			mTextAdv[c] = mFont->glyph(c)->advX;

			charactersBuffer[i*6 + 0].u = sx;
			charactersBuffer[i*6 + 0].v = sy;
			charactersBuffer[i*6 + 0].x = 0.0f;
			charactersBuffer[i*6 + 0].y = fy;

			charactersBuffer[i*6 + 1].u = sx+texMaxX;
			charactersBuffer[i*6 + 1].v = sy+texMaxY;
			charactersBuffer[i*6 + 1].x = 0.0f+width;
			charactersBuffer[i*6 + 1].y = fy+height;

			charactersBuffer[i*6 + 2].u = sx+texMaxX;
			charactersBuffer[i*6 + 2].v = sy;
			charactersBuffer[i*6 + 2].x = 0.0f+width;
			charactersBuffer[i*6 + 2].y = fy;
			
			charactersBuffer[i*6 + 3].u = sx;
			charactersBuffer[i*6 + 3].v = sy;
			charactersBuffer[i*6 + 3].x = 0.0f;
			charactersBuffer[i*6 + 3].y = fy;

			charactersBuffer[i*6 + 4].u = sx;
			charactersBuffer[i*6 + 4].v = sy+texMaxY;
			charactersBuffer[i*6 + 4].x = 0.0f;
			charactersBuffer[i*6 + 4].y = fy+height;

			charactersBuffer[i*6 + 5].u = sx+texMaxX;
			charactersBuffer[i*6 + 5].v = sy+texMaxY;
			charactersBuffer[i*6 + 5].x = 0.0f+width;
			charactersBuffer[i*6 + 5].y = fy+height;
		}

		glGenBuffers( 1, &mTextVBO );
		glBindBuffer( GL_ARRAY_BUFFER, mTextVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 6 * 256, charactersBuffer, GL_STATIC_DRAW );
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
	gDebug() << "program compile : " << log << "\n";

	glUseProgram( target->mShader );
	target->mMatrixProjectionID = glGetUniformLocation( target->mShader, "ge_ProjectionMatrix" );
	target->mColorID = glGetUniformLocation( target->mShader, "color" );
	target->mOffsetID = glGetUniformLocation( target->mShader, "offset" );
	glUniformMatrix4fv( target->mMatrixProjectionID, 1, GL_FALSE, mMatrixProjection->constData() );

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, 0 );
}


int RendererHUD::LoadVertexShader( Shader* target, const void* data, size_t size )
{
	mReady = false;
	if ( target->mVertexShader ) {
		glDeleteShader( target->mVertexShader );
	}

	target->mVertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( target->mVertexShader, 1, (const char**)&data, NULL );
	glCompileShader( target->mVertexShader );
	char log[4096] = "";
	int logsize = 4096;
	glGetShaderInfoLog( target->mVertexShader, logsize, &logsize, log );
	gDebug() << "vertex compile : " << log << "\n";

	return 0;
}


int RendererHUD::LoadFragmentShader( Shader* target, const void* data, size_t size )
{
	mReady = false;
	if ( target->mFragmentShader ) {
		glDeleteShader( target->mFragmentShader );
	}

	target->mFragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( target->mFragmentShader, 1, (const char**)&data, NULL );
	glCompileShader( target->mFragmentShader );
	char log[4096] = "";
	int logsize = 4096;
	glGetShaderInfoLog( target->mFragmentShader, logsize, &logsize, log );
	gDebug() << "fragment compile : " << log << "\n";

	return 0;
}


uint8_t* RendererHUD::loadShader( const std::string& filename, size_t* sz )
{
	File* file = new File( filename, File::READ );

	size_t size = file->Seek( 0, File::END );
	file->Rewind();

	uint8_t* data = (uint8_t*)mInstance->Malloc( size + 1 );

	file->Read( data, size );
	data[size] = 0;
	delete file;

	if(sz){
		*sz = size;
	}
	return data;
}
