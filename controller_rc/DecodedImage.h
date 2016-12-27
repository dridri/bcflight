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

#ifndef DECODEDIMAGE_H
#define DECODEDIMAGE_H

#include <GLES2/gl2.h>

class RendererHUD;

class DecodedImage
{
public:
	DecodedImage( uint32_t width, uint32_t height, uint32_t backcolor );

	GLuint texture() const { return mTexture; }
	void setDrawCoordinates( uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height );
	void Draw( RendererHUD* renderer );

protected:
	GLuint mTexture;
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mOffsetX;
	uint32_t mOffsety;
	uint32_t mDrawWidth;
	uint32_t mDrawHeight;
};

#endif // DECODEDIMAGE_H
