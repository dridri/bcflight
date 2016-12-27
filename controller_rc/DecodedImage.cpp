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

#include "DecodedImage.h"
#include "RendererHUD.h"

DecodedImage::DecodedImage( uint32_t width, uint32_t height, uint32_t backcolor )
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


void DecodedImage::setDrawCoordinates( uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height )
{
	mOffsetX = offset_x;
	mOffsety = offset_y;
	mDrawWidth = width;
	mDrawHeight = height;
}


void DecodedImage::Draw( RendererHUD* renderer )
{
	renderer->RenderQuadTexture( mTexture, mOffsetX, mOffsety, mDrawWidth, mDrawHeight, false, true );
}
