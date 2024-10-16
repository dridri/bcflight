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

#ifndef QUATERNION_H
#define QUATERNION_H

#include "Matrix.h"
#include "Vector.h"

class Quaternion : public Vector<float, 4>
{
public:
	Quaternion( float x = 0.0, float y = 0.0f, float z = 0.0f, float w = 0.0f );
	~Quaternion();

	void normalize();

	Matrix matrix();
	Matrix inverseMatrix();

	Quaternion operator+( const Quaternion& v ) const;
	Quaternion operator-( const Quaternion& v ) const;
	Quaternion operator*( float im ) const;
	Quaternion operator*( const Quaternion& v ) const;
};

inline Quaternion operator*( float im, const Quaternion& v )
{
	Quaternion ret;

	ret.x = v.x * im;
	ret.y = v.y * im;
	ret.z = v.z * im;
	ret.w = v.w * im;

	return ret;
}

#endif // QUATERNION_H
