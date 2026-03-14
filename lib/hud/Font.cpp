#include "Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <cstring>
#include <climits>
#include <iostream>
#include <bits/stdc++.h>
#include <Debug.h>

static constexpr uint32_t kGridCells = 16;

// Next power-of-two >= n
static uint32_t nextPow2( uint32_t n ) {
	n--;
	n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
	return n + 1;
}

Font::Font()
	: mLineHeight( 0 )
	, mAtlasSize( 0 )
	, mBlurRadius( 0 )
	, mGLTextureID( 0 )
	, mGLVBO( 0 )
{
	memset( adv,     0, sizeof(adv) );
	memset( mUvX,    0, sizeof(mUvX) );
	memset( mUvY,    0, sizeof(mUvY) );
	memset( mUvW,    0, sizeof(mUvW) );
	memset( mUvH,    0, sizeof(mUvH) );
	memset( mGlyphW,       0, sizeof(mGlyphW) );
	memset( mGlyphH,       0, sizeof(mGlyphH) );
	memset( mGlyphBitmapH, 0, sizeof(mGlyphBitmapH) );
	memset( mGlyphYOffset, 0, sizeof(mGlyphYOffset) );
}


Font::Font( const std::string& ttfPath, uint32_t pixelSize )
	: Font()
{
	uint64_t t0 = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	uint64_t t = t0;
	const uint32_t kCellSize = nextPow2( (uint32_t)( pixelSize * 2.0f ) );
	mAtlasSize = kGridCells * kCellSize;
	mAtlasData.assign( mAtlasSize * mAtlasSize, 0 );

	FT_Library library;
	if ( FT_Init_FreeType( &library ) ) {
		std::cerr << "Font: FT_Init_FreeType failed\n";
		return;
	}

	FT_Face face;
	if ( FT_New_Face( library, ttfPath.c_str(), 0, &face ) ) {
		std::cerr << "Font: FT_New_Face failed for \"" << ttfPath << "\"\n";
		FT_Done_FreeType( library );
		return;
	}

	FT_Set_Pixel_Sizes( face, 0, pixelSize );

	t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	gDebug() << "Freetype init time : " << ( t - t0 ) / 1000000.0f << " ms";
	t0 = t;

	// Ascender in pixels: distance from baseline to top of cell
	const int ascender  = (int)( face->size->metrics.ascender  >> 6 );
	const int descender = (int)( face->size->metrics.descender >> 6 ); // negative
	mLineHeight = ascender - descender;

	const float invAtlas = 1.0f / (float)mAtlasSize;

	FT_Load_Char( face, 'M', FT_LOAD_RENDER );
	mCellOffsetX = ( kCellSize - std::min(face->glyph->bitmap.width, (uint32_t)face->glyph->advance.x >> 6) ) / 2;
	mCellOffsetY = ( kCellSize - face->size->metrics.height / 64 ) / 2;

	for ( uint32_t i = 0; i < 256; i++ ) {
		if ( FT_Load_Char( face, i, FT_LOAD_RENDER ) ) {
			// unmapped character — leave zeroed
			adv[i] = pixelSize / 2;
			continue;
		}

		FT_GlyphSlot glyph  = face->glyph;
		FT_Bitmap&   bitmap = glyph->bitmap;

		uint32_t bw = bitmap.width;
		uint32_t bh = bitmap.rows;

		// Cell top-left in atlas
		uint32_t cellX = ( i % kGridCells ) * kCellSize;
		uint32_t cellY = ( i / kGridCells ) * kCellSize;

		// Vertical offset within cell: place bitmap relative to baseline
		int yOffset = ascender - (int)glyph->bitmap_top - 2;
		if ( yOffset < 0 ) yOffset = 0;

		// cell offset : center the bitmap within the cell to ensure outline and blur are not clipped or overlapping adjacent cells
		int cellOffsetX = ( kCellSize - bw ) / 2;
		int cellOffsetY = ( kCellSize - bh) / 2;

		// Copy bitmap into atlas at baseline-aligned position
		for ( uint32_t row = 0; row < bh; row++ ) {
			uint32_t destRow = /*(uint32_t)yOffset + */row + cellOffsetY;
			if ( destRow >= kCellSize ) break;
			for ( uint32_t col = 0; col < bw && col < kCellSize; col++ ) {
				uint32_t gray = bitmap.buffer[ row * bitmap.pitch + col ];
				// fill in R channel, outline will be written in G channel after dilation
				mAtlasData[ ( cellY + destRow ) * mAtlasSize + ( cellX + col + cellOffsetX ) ] = gray;
			}
		}

		adv[i]           = (int)std::max( (uint32_t)glyph->advance.x >> 6 + 1, (uint32_t)bw + 2 );
		mGlyphW[i]       = (int)bw;
		mGlyphH[i]       = (int)kCellSize;
		mGlyphBitmapH[i] = (int)bh;
		mGlyphYOffset[i] = yOffset;

		mUvX[i] = (float)cellX    * invAtlas;
		mUvY[i] = (float)cellY    * invAtlas;
		mUvW[i] = (float)kCellSize/*mGlyphW[i]*/ * invAtlas;
		mUvH[i] = (float)kCellSize  * invAtlas;
	}

	t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	gDebug() << "Font atlas generation time : " << ( t - t0 ) / 1000000.0f << " ms";
	t0 = t;

	const uint32_t atlasSize   = mAtlasSize;
	const uint32_t totalPixels = atlasSize * atlasSize;
	mBlurRadius = pixelSize / 4;

	// Outline dilation pass (R → G): 3x3 max
	for ( uint32_t y = 0; y < totalPixels; y += atlasSize ) {
		const uint32_t y0 = ( y > 0 ) ? y - atlasSize : 0;
		const uint32_t y1 = ( y + atlasSize < totalPixels ) ? y + atlasSize : y;
		for ( uint32_t x = 0; x < atlasSize; x++ ) {
			const uint32_t x0 = ( x > 0 ) ? x - 1 : 0;
			const uint32_t x1 = ( x + 1 < atlasSize ) ? x + 1 : x;
			uint8_t outline = 0;
			for ( uint32_t sy = y0; sy <= y1; sy += atlasSize ) {
				for ( uint32_t sx = x0; sx <= x1; sx++ ) {
					uint8_t v = (uint8_t)( mAtlasData[ sy + sx ] & 0xFF );
					if ( v > outline ) outline = v;
				}
			}
			if ( outline > 0 ) {
				mAtlasData[ y + x ] |= (uint32_t)outline << 8;
			}
		}
	}
/*
	// Debug : draw vertical and horizontal grid lines
	for ( uint32_t i = 0; i <= kGridCells; i++ ) {
		uint32_t lineX = i * kCellSize;
		uint32_t lineY = i * kCellSize;
		for ( uint32_t y = 0; y < atlasSize; y++ ) {
			if ( lineX < atlasSize ) {
				mAtlasData[ y * atlasSize + lineX ] = 0xFFFF0000;
			}
		}
		for ( uint32_t x = 0; x < atlasSize; x++ ) {
			if ( lineY < atlasSize ) {
				mAtlasData[ lineY * atlasSize + x ] = 0xFFFF0000;
			}
		}
	}
*/
	FT_Done_Face( face );
	FT_Done_FreeType( library );
}


Font* Font::fromRawPixels( const uint32_t* pixels, uint32_t atlasW, uint32_t atlasH, uint32_t cellW, uint32_t cellH, uint32_t targetSize )
{
	Font* f = new Font();
	f->mAtlasSize = atlasW; // square atlas assumed
	f->mAtlasData.assign( pixels, pixels + atlasW * atlasH );
	f->mLineHeight = (int)( cellH * ( (float)targetSize / (float)cellW ) );

	const float rx = 1.0f / (float)atlasW;
	const float ry = 1.0f / (float)atlasH;
	const float scale = (float)targetSize / (float)cellW;

	for ( uint32_t i = 0; i < 256; i++ ) {
		uint32_t xbase = cellW * ( i % 16 );
		uint32_t ybase = cellH * ( i / 16 );
		uint32_t xend  = xbase + cellW;
		uint32_t yend  = ybase + cellH;

		// Scan pixel width (same logic as old CharacterWidth)
		uint32_t xfirst = 0, xlast = 0;
		for ( uint32_t y = ybase; y < yend; y++ ) {
			for ( uint32_t x = xbase; x < xend; x++ ) {
				if ( pixels[x + y * atlasW] != 0 ) {
					if ( xfirst == 0 || x < xfirst ) {
						xfirst = x;
					}
					if ( x + 1 < xend && pixels[x+1 + y * atlasW] == 0 ) {
						if ( xfirst != 0 && x > xlast ) {
							xlast = x;
						}
					}
				}
			}
		}
		uint32_t charW = ( xfirst > 0 && xlast > 0 ) ? xlast - xfirst + 1 : cellW;

		f->mUvX[i] = (float)xbase * rx;
		f->mUvY[i] = (float)ybase * ry;
		f->mUvW[i] = (float)charW * rx;
		f->mUvH[i] = (float)cellH * ry;
		f->mGlyphW[i]       = (int)( charW * scale );
		f->mGlyphH[i]       = (int)( cellH * scale );
		f->mGlyphBitmapH[i] = f->mGlyphH[i];
		f->adv[i]           = f->mGlyphW[i] + 1;
	}

	return f;
}


void Font::measureText( const std::string& text, int* width, int* height, int* yOffset, float scale ) const
{
	if ( text.empty() ) {
		*width = 0;
		*height = 0;
		if ( yOffset ) *yOffset = 0;
		return;
	}

	int maxW      = 0;
	int lineW     = 0;
	int minYOfs   = INT_MAX;
	int maxBottom = 0;
	int lines     = 1;

	for ( size_t i = 0; i < text.size(); i++ ) {
		uint8_t c = (uint8_t)text[i];
		if ( c == '\n' ) {
			if ( lineW > maxW ) maxW = lineW;
			lineW = 0;
			lines++;
			continue;
		}
		bool isLast = ( i + 1 == text.size() || text[i+1] == '\n' );
		lineW += (int)( ( isLast ? mGlyphW[c] : adv[c] ) * scale );
		int top    = (int)( mGlyphYOffset[c] * scale );
		int bottom = (int)( ( mGlyphYOffset[c] + mGlyphBitmapH[c] ) * scale );
		if ( top    < minYOfs   ) minYOfs   = top;
		if ( bottom > maxBottom ) maxBottom = bottom;
	}
	if ( lineW > maxW ) maxW = lineW;
	if ( minYOfs == INT_MAX ) minYOfs = 0;

	*width  = maxW;
	*height = ( maxBottom - minYOfs ) + ( lines - 1 ) * (int)( mLineHeight * scale );
	if ( yOffset ) *yOffset = minYOfs;
}


Font::~Font()
{
	if ( mGLVBO )       glDeleteBuffers( 1, &mGLVBO );
	if ( mGLTextureID ) glDeleteTextures( 1, &mGLTextureID );
}


void Font::uploadGL( uint32_t spaceAdvance )
{
	uint64_t t0 = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	if ( spaceAdvance > 0 ) {
		adv[' '] = (int)spaceAdvance;
	}

	const uint32_t kCellSize = mAtlasSize / kGridCells;
	// const float hCenter = (float)mCellOffsetX;
	// const float vCenter = (float)mCellOffsetY;

	// Build character VBO — layout: u, v, x, y (matches RendererHUD::FastVertex)
	struct Vertex { float u, v, x, y; };
	Vertex buf[6 * 256];
	for ( uint32_t i = 0; i < 256; i++ ) {
		float hCenter = ( kCellSize - std::min(mGlyphW[i], adv[i]) ) / 2;
		float vCenter = ( kCellSize - mGlyphBitmapH[i] ) / 2 - mGlyphYOffset[i];
		float sx  = mUvX[i], sy  = mUvY[i];
		float tw  = mUvW[i], th  = mUvH[i];
		float w   = tw * mAtlasSize; // (float)mGlyphW[i];
		float h   = th * mAtlasSize; // (float)mGlyphH[i];
		buf[i*6+0] = { sx,    sy,    -hCenter,     -vCenter };
		buf[i*6+1] = { sx+tw, sy+th, -hCenter + w, -vCenter + h };
		buf[i*6+2] = { sx+tw, sy,    -hCenter + w, -vCenter };
		buf[i*6+3] = { sx,    sy,    -hCenter,     -vCenter };
		buf[i*6+4] = { sx,    sy+th, -hCenter,     -vCenter + h };
		buf[i*6+5] = { sx+tw, sy+th, -hCenter + w, -vCenter + h };
	}

	glGenBuffers( 1, &mGLVBO );
	glBindBuffer( GL_ARRAY_BUFFER, mGLVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW );

	// Pre-fill B channel with characters pixels (from R channel) for use in blur shader
	if ( mBlurRadius > 0 ) {
		for ( uint32_t i = 0; i < mAtlasData.size(); i++ ) {
			mAtlasData[i] |= ( mAtlasData[i] & 0xFF ) << 16;
		}
	}

	// Upload atlas texture (R = fill, G = outline, B = 0 for now)
	// glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glGenTextures( 1, &mGLTextureID );
	glBindTexture( GL_TEXTURE_2D, mGLTextureID );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, mAtlasSize, mAtlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, mAtlasData.data() );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	// GPU gaussian blur: R channel → B channel via 2-pass separable blur in FBO
	if ( mBlurRadius > 0 ) {
		const int32_t kBlurDiam = 2 * mBlurRadius + 1;
		const float sigma  = (float)mBlurRadius / 2.0f;
		const float sigma2 = 2.0f * sigma * sigma;
		float texelSize = 1.0f / (float)mAtlasSize;

		// Compute 1D gaussian weights
		float gaussW[kBlurDiam];
		float gaussSum = 0.0f;
		for ( int32_t i = 0; i < kBlurDiam; i++ ) {
			float d = (float)( i - mBlurRadius );
			gaussW[i] = std::exp( -( d * d ) / sigma2 );
			gaussSum += gaussW[i];
		}
		for ( int32_t i = 0; i < kBlurDiam; i++ ) gaussW[i] /= gaussSum;

		const char* blurVS =
			"attribute vec2 pos;\n"
			"varying vec2 vUV;\n"
			"void main() {\n"
			"  vUV = pos * 0.5 + 0.5;\n"
			"  gl_Position = vec4(pos, 0.0, 1.0);\n"
			"}\n";

		// Separable blur shader: direction via uniform vec2
		std::string blurFS =
			"precision mediump float;\n"
			"varying vec2 vUV;\n"
			"uniform sampler2D tex;\n"
			"uniform vec2 dir;\n"
			"uniform float weights[" + std::to_string(kBlurDiam) + "];\n"
			"void main() {\n"
			"  float acc = 0.0;\n"
			"  for (int i = 0; i < " + std::to_string(kBlurDiam) + "; i++) {\n"
			"    vec2 ofs = dir * float(i - " + std::to_string(mBlurRadius) + ");\n"
			"    acc += texture2D(tex, vUV + ofs).b * weights[i];\n"
			"  }\n"
			"  vec4 c = texture2D(tex, vUV);\n"
			"  gl_FragColor = vec4(c.r, c.g, acc, 1.0);\n"
			"}\n";

		// Compile & link
		auto compileShader = []( GLenum type, const char* src ) -> GLuint {
			GLuint s = glCreateShader( type );
			glShaderSource( s, 1, &src, nullptr );
			glCompileShader( s );
			GLint ok = 0;
			glGetShaderiv( s, GL_COMPILE_STATUS, &ok );
			if ( !ok ) {
				char log[512];
				glGetShaderInfoLog( s, 512, nullptr, log );
				gDebug() << "Font blur shader error: " << log;
			}
			return s;
		};

		GLuint vsObj = compileShader( GL_VERTEX_SHADER, blurVS );
		GLuint fsObj = compileShader( GL_FRAGMENT_SHADER, blurFS.c_str() );

		GLuint prog = glCreateProgram();
		glAttachShader( prog, vsObj );
		glAttachShader( prog, fsObj );
		glBindAttribLocation( prog, 0, "pos" );
		glLinkProgram( prog );

		GLint dirLoc = glGetUniformLocation( prog, "dir" );
		GLint weightsLoc = glGetUniformLocation( prog, "weights[0]" );

		// Fullscreen quad
		float quadVerts[] = { -1,-1, 1,-1, -1,1, 1,1 };
		GLuint quadVBO;
		glGenBuffers( 1, &quadVBO );
		glBindBuffer( GL_ARRAY_BUFFER, quadVBO );
		glBufferData( GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW );

		// Save GL state
		GLint prevViewport[4];
		glGetIntegerv( GL_VIEWPORT, prevViewport );

		// Two textures for ping-pong: mGLTextureID → tmpTex (h-blur) → outTex (v-blur)
		GLuint fbo;
		glGenFramebuffers( 1, &fbo );

		auto makeBlurTex = [&]() -> GLuint {
			GLuint t;
			glGenTextures( 1, &t );
			glBindTexture( GL_TEXTURE_2D, t );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, mAtlasSize, mAtlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			return t;
		};
		GLuint tmpTex = makeBlurTex();
		GLuint outTex = makeBlurTex();

		glBindFramebuffer( GL_FRAMEBUFFER, fbo );
		glViewport( 0, 0, mAtlasSize, mAtlasSize );
		glUseProgram( prog );
		glUniform1fv( weightsLoc, kBlurDiam, gaussW );
		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0 );

		// Pass 1: H-blur — read mGLTextureID.r, write tmpTex.b
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmpTex, 0 );
		glBindTexture( GL_TEXTURE_2D, mGLTextureID );
		glUniform2f( dirLoc, texelSize, 0.0f );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		// Pass 2: V-blur — read tmpTex.r + original R,G → outTex
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex, 0 );
		glBindTexture( GL_TEXTURE_2D, tmpTex );
		glUniform2f( dirLoc, 0.0f, texelSize );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		// Swap: outTex becomes our atlas, set LINEAR for rendering
		glDeleteTextures( 1, &mGLTextureID );
		mGLTextureID = outTex;
		glBindTexture( GL_TEXTURE_2D, mGLTextureID );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		glUseProgram( 0 );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glViewport( prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3] );

		// Cleanup
		glDeleteBuffers( 1, &quadVBO );
		glDeleteFramebuffers( 1, &fbo );
		glDeleteTextures( 1, &tmpTex );
		glDeleteProgram( prog );
		glDeleteShader( vsObj );
		glDeleteShader( fsObj );
	}

	// Free CPU atlas data
	mAtlasData.clear();
	mAtlasData.shrink_to_fit();

	t0 = std::chrono::high_resolution_clock::now().time_since_epoch().count() - t0;
	gDebug() << "Font GL upload time : " << t0 / 1000000.0f << " ms";
}
