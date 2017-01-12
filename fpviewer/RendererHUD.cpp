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
#include <cmath>
#include <Thread.h>
#include "RendererHUD.h"

static const char hud_flat_vertices_shader[] =
R"(	#define in attribute
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
		ge_Position.y = ge_Position.y * ( 720.0 / ( 1280.0 / 2.0 ) );
	})"
;

static const char hud_flat_fragment_shader[] =
R"(	#define in varying
	#define ge_FragColor gl_FragColor
	uniform sampler2D ge_Texture0;

	uniform vec4 color;
	in vec2 ge_TextureCoord;

	void main()
	{
// 		uniform float kernel[9] = float[]( 0, -1, 0, -1, 5, -1, 0, -1, 0 );
		ge_FragColor = vec4(0.0);
		ge_FragColor += 5.0 * texture2D( ge_Texture0, ge_TextureCoord.xy );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2(-0.001, 0.0 ) );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2( 0.001, 0.0 ) );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2( 0.0,-0.001 ) );
		ge_FragColor += -1.0 * texture2D( ge_Texture0, ge_TextureCoord.xy + vec2( 0.0, 0.001 ) );

		ge_FragColor *= color;
		ge_FragColor.a = 1.0;
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


RendererHUD::RendererHUD( int width, int height )
	: mWidth( width )
	, mHeight( height )
	, mStereo( true )
	, mNightMode( false )
	, m3DStrength( 0.004f )
	, mBlinkingViews( false )
	, mMatrixProjection( new Matrix() )
	, mQuadVBO( 0 )
	, mFontTexture( nullptr )
	, mFontSize( 28 )
	, mTextShader{ 0 }
{
	mMatrixProjection->Orthogonal( 0.0, 1280, 720, 0.0, -2049.0, 2049.0 );
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
	glUseProgram( 0 );

	LoadVertexShader( &mTextShader, hud_text_vertices_shader, sizeof(hud_text_vertices_shader) + 1 );
	LoadFragmentShader( &mTextShader, hud_text_fragment_shader, sizeof(hud_text_fragment_shader) + 1 );
	createPipeline( &mTextShader );
	glUseProgram( mTextShader.mShader );
	glEnableVertexAttribArray( mTextShader.mVertexTexcoordID );
	glEnableVertexAttribArray( mTextShader.mVertexPositionID );
	glUseProgram( 0 );

	glGenBuffers( 1, &mQuadVBO );
	glBindBuffer( GL_ARRAY_BUFFER, mQuadVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof(FastVertex) * 6, nullptr, GL_STATIC_DRAW );

	{
		mFontTexture = LoadTexture( "data/font.png" );
		const float rx = 1.0f / (float)mFontTexture->width;
		const float ry = 1.0f / (float)mFontTexture->height;
		FastVertex charactersBuffer[6 * 256];
		memset( charactersBuffer, 0, sizeof( charactersBuffer ) );
		for ( uint32_t i = 0; i < 256; i++ ) {
			uint8_t c = i;
			float sx = (float)( i % 16 ) * ( mFontTexture->width / 16 ) * rx;
			float sy = (float)( i / 16 ) * ( mFontTexture->height / 16 ) * ry;
			float width = CharacterWidth( mFontTexture, i );
			float height = CharacterHeight( mFontTexture, i );
			float texMaxX = width * rx;
			float texMaxY = height * ry;
			float fy = (float)0.0f;// - CharacterYOffset( mFontTexture, i );
			mTextAdv[c] = (int)width + 1;

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
		mTextAdv[' '] = mFontSize * 0.35f;

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
	if ( Thread::GetTick() - mHUDTick >= 1000 ) {
		setNightMode( false ); // TODO : detect night from framebuffer in videostats
		mBlinkingViews = !mBlinkingViews;
		mHUDTick = Thread::GetTick();
	}
}


void RendererHUD::RenderQuadTexture( GLuint textureID, int x, int y, int width, int height, bool hmirror, bool vmirror )
{
	glUseProgram( mFlatShader.mShader );
	glUniform4f( mFlatShader.mColorID, 1.0f, 1.0f, 1.0f, 1.0f );
	glUniform1f( mFlatShader.mScaleID, 1.0f );
	glBindTexture( GL_TEXTURE_2D, textureID );

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, textureID );
	glUniform1i( glGetUniformLocation( mFlatShader.mShader, "ge_Texture0" ), 0 );

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

	glUniform2f( mTextShader.mOffsetID, 1280.0f * +m3DStrength, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	glUniform2f( mTextShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f, 0.0f );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
}


void RendererHUD::RenderText( int x, int y, const std::string& text, uint32_t color, float size, bool hcenter )
{
	RenderText( x, y, text, Vector4f( (float)( color & 0xFF ) / 256.0f, (float)( ( color >> 8 ) & 0xFF ) / 256.0f, (float)( ( color >> 16 ) & 0xFF ) / 256.0f, 1.0f ), size, hcenter );
}


void RendererHUD::RenderText( int x, int y, const std::string& text, const Vector4f& _color, float size, bool hcenter )
{
	y += mFontSize * size * 0.2f;

	glUseProgram( mTextShader.mShader );
	Vector4f color = _color;
	if ( mNightMode ) {
		color.x *= 0.5f;
		color.y = std::min( 1.0f, color.y * 1.0f );
		color.z *= 0.5f;
	}
	glUniform4f( mTextShader.mColorID, color.x, color.y, color.z, color.w );
	glUniform1f( mTextShader.mScaleID, size );
	glBindTexture( GL_TEXTURE_2D, mFontTexture->glID );

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, mFontTexture->glID );
	glUniform1i( glGetUniformLocation( mTextShader.mShader, "ge_Texture0" ), 0 );

	glBindBuffer( GL_ARRAY_BUFFER, mTextVBO );
	glVertexAttribPointer( mTextShader.mVertexTexcoordID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( 0 ) );
	glVertexAttribPointer( mTextShader.mVertexPositionID, 2, GL_FLOAT, GL_FALSE, sizeof( FastVertex ), (void*)( sizeof( float ) * 2 ) );

	if ( hcenter ) {
		int w = 0;
		for ( uint32_t i = 0; i < text.length(); i++ ) {
			
			w += mTextAdv[ (uint8_t)( (int)text.data()[i] ) ] * size;
		}
		x -= w / 2.0f;
	}

	for ( uint32_t i = 0; i < text.length(); i++ ) {
		uint8_t c = (uint8_t)( (int)text.data()[i] );
		if ( c > 0 and c < 128 ) {
			glUniform2f( mTextShader.mOffsetID, 1280.0f * +m3DStrength + x, y );
			glDrawArrays( GL_TRIANGLES, c * 6, 6 );
			glUniform2f( mTextShader.mOffsetID, 1280.0f * -m3DStrength + 1280.0f / 2.0f + x, y );
			glDrawArrays( GL_TRIANGLES, c * 6, 6 );
		}
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
	std::cout << "program compile : " << log << "\n";

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
		x += mTextAdv[ (uint8_t)str[i] ];
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


RendererHUD::Texture* RendererHUD::LoadTexture( const std::string& filename )
{
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
		LoadPNG( tex, filename );
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


static void png_read_from_ifstream( png_structp png_ptr, png_bytep data, png_size_t length )
{
	std::ifstream* file = ( std::ifstream* )png_get_io_ptr( png_ptr );
	file->read( (char*)data, length );
}


void RendererHUD::LoadPNG( Texture* tex, const std::string& filename )
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height, x, y;
	int bit_depth, color_type, interlace_type;
	uint32_t* line;

	std::ifstream file( filename, std::ios_base::in | std::ios_base::binary );

	uint8_t magic[8] = { 0x0 };
	file.read( (char*)magic, 8 );
	file.seekg( 0, file.beg );
#ifdef png_check_sig
	if ( !png_check_sig( magic, 8 ) ) {
		return;
	}
#endif

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
