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

#include <gammaengine/Image.h>
#include <GLES2/gl2.h>

using namespace GE;

class DecodedImage : public Image
{
public:
	DecodedImage( Instance* instance, uint32_t width, uint32_t height, uint32_t backcolor );

protected:
	GLuint mTexture;
};

#endif // DECODEDIMAGE_H
