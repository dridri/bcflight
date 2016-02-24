#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cmath>
#include <gammaengine/File.h>
#include <gammaengine/Debug.h>
#include <gammaengine/Time.h>
#include "RendererHUD.h"


static const char hud_text_vertices_shader[] =
R"(	#define in attribute
	#define out varying
	#define ge_Position gl_Position
	in vec2 ge_VertexTexcoord;
	in vec2 ge_VertexPosition;

	uniform mat4 ge_ProjectionMatrix;
	uniform vec2 offset;
	uniform float scale;
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
		vec2 pos = offset + ge_VertexPosition.xy * scale;
		pos = VR_Distort( pos );
		ge_Position = ge_ProjectionMatrix * vec4(pos, 0.0, 1.0);
		ge_Position.y = ge_Position.y * ( 720.0 / ( 1280.0 / 2.0 ) );
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


Vector2f RendererHUD::VR_Distort( const Vector2f& coords )
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
	, mNightMode( false )
	, m3DStrength( 0.004f )
	, mBlinkingViews( false )
	, mHUDTimer( Timer() )
	, mMatrixProjection( new Matrix() )
	, mTextShader{ 0 }
{
	mHUDTimer.Start();

	mMatrixProjection->Orthogonal( 0.0, 1280, 720, 0.0, -2049.0, 2049.0 );
	glDisable( GL_DEPTH_TEST );
	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glLineWidth( 2.0f );

	LoadVertexShader( &mTextShader, hud_text_vertices_shader, sizeof(hud_text_vertices_shader) + 1 );
	LoadFragmentShader( &mTextShader, hud_text_fragment_shader, sizeof(hud_text_fragment_shader) + 1 );
	createPipeline( &mTextShader );
	glUseProgram( mTextShader.mShader );
	glEnableVertexAttribArray( mTextShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mTextShader.mVertexPositionID );
	glUseProgram( 0 );

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


RendererHUD::~RendererHUD()
{
}


void RendererHUD::PreRender( VideoStats* videostats )
{
	if ( mHUDTimer.ellapsed() >= 1000 ) {
		setNightMode( false ); // TODO : detect night from framebuffer in videostats
		mBlinkingViews = !mBlinkingViews;
		mHUDTimer.Stop();
		mHUDTimer.Start();
	}
}


void RendererHUD::RenderText( int x, int y, const std::string& text, uint32_t color, float size, bool hcenter )
{
	RenderText( x, y, text, Vector4f( (float)( color & 0xFF ) / 256.0f, (float)( ( color >> 8 ) & 0xFF ) / 256.0f, (float)( ( color >> 16 ) & 0xFF ) / 256.0f, 1.0f ), size, hcenter );
}


void RendererHUD::RenderText( int x, int y, const std::string& text, const Vector4f& _color, float size, bool hcenter )
{
	// TODO : use size
	y += mFont->size();

	glUseProgram( mTextShader.mShader );
	Vector4f color = _color;
	if ( mNightMode ) {
		color.x *= 0.5f;
		color.y = std::min( 1.0f, color.y * 1.0f );
		color.z *= 0.5f;
	}
	glUniform4f( mTextShader.mColorID, color.x, color.y, color.z, color.w );
	glUniform1f( mTextShader.mScaleID, size );
	glBindTexture( GL_TEXTURE_2D, mFont->texture()->serverReference( mInstance ) );

	glBindBuffer( GL_ARRAY_BUFFER, mTextVBO );
	glVertexAttribPointer( mTextShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mTextShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	if ( hcenter ) {
		int w = 0;
		for ( uint32_t i = 0; i < text.length(); i++ ) {
			w += mTextAdv[ (uint8_t)( (int)text.data()[i] ) ] * size;
		}
		x -= w / 2;
	}

	for ( uint32_t i = 0; i < text.length(); i++ ) {
		uint8_t c = (uint8_t)( (int)text.data()[i] );
		glUniform2f( mTextShader.mOffsetID, 1280.0f * +m3DStrength + x, y );
		glDrawArrays( GL_TRIANGLES, c * 6, 6 );
		glUniform2f( mTextShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f + x, y );
		glDrawArrays( GL_TRIANGLES, c * 6, 6 );
		x += mTextAdv[ c ] * size;
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
	gDebug() << "vertex compile : " << log << "\n";

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
