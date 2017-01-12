#include "GLImage.h"
#include <RendererHUD.h>

GLImage::GLImage( uint32_t width, uint32_t height )
	: mWidth( width )
	, mHeight( height )
	, mOffsetX( 0 )
	, mOffsety( 0 )
	, mDrawWidth( width )
	, mDrawHeight( height )
{

	glGenTextures( 1, &mTexture );

	glBindTexture( GL_TEXTURE_2D, mTexture );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}


void GLImage::setDrawCoordinates( uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height )
{
	mOffsetX = offset_x;
	mOffsety = offset_y;
	mDrawWidth = width;
	mDrawHeight = height;
}


void GLImage::Draw( RendererHUD* renderer )
{
	renderer->RenderQuadTexture( mTexture, mOffsetX, mOffsety, mDrawWidth, mDrawHeight, false, true );
}