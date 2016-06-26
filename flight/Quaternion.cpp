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
#include <cmath>
#include "Quaternion.h"
#include "Debug.h"

using namespace GE;

Quaternion::Quaternion( float x, float y, float z, float w )
	: Vector< float, 4 >( x, y, z, w )
{
}


Quaternion::~Quaternion()
{
}


void Quaternion::normalize()
{
	float l = std::sqrt( x * x + y * y + z * z + w * w );
	if ( l == 0.0f ) {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 1.0f;
	} else {
		float il = 1 / l;
		x *= il;
		y *= il;
		z *= il;
		w *= il;
	}
}


Quaternion Quaternion::operator+( const Quaternion& v ) const
{
	Quaternion ret;

	ret.x = x + v.x;
	ret.y = y + v.y;
	ret.z = z + v.z;
	ret.w = w + v.w;

	return ret;
}


Quaternion Quaternion::operator-( const Quaternion& v ) const 
{
	Quaternion ret;

	ret.x = x - v.x;
	ret.y = y - v.y;
	ret.z = z - v.z;
	ret.w = w - v.w;

	return ret;
}


Quaternion Quaternion::operator*( float im ) const
{
	Quaternion ret;

	ret.x = x * im;
	ret.y = y * im;
	ret.z = z * im;
	ret.w = w * im;

	return ret;
}


Quaternion Quaternion::operator*( const Quaternion& v ) const
{
	Quaternion ret;

	ret.x = w * v.x + x * v.w + y * v.z - z * v.y;
	ret.y = w * v.y - x * v.z + y * v.w + z * v.x;
	ret.z = w * v.z + x * v.y - y * v.x + z * v.w;
	ret.w = w * v.w - x * v.x - y * v.y - z * v.z;

	return ret;
}


Matrix Quaternion::matrix()
{
	Matrix ret;

	float fTx  = 2.0f * x;
	float fTy  = 2.0f * y;
	float fTz  = 2.0f * z;
	float fTwx = fTx * w;
	float fTwy = fTy * w;
	float fTwz = fTz * w;
	float fTxx = fTx * x;
	float fTxy = fTy * x;
	float fTxz = fTz * x;
	float fTyy = fTy * y;
	float fTyz = fTz * y;
	float fTzz = fTz * z;

	ret.data()[0] = 1.0f - ( fTyy + fTzz );
	ret.data()[1] = fTxy - fTwz;
	ret.data()[2] = fTxz + fTwy;
	ret.data()[4] = fTxy + fTwz;
	ret.data()[5] = 1.0f - ( fTxx + fTzz );
	ret.data()[6] = fTyz - fTwx;
	ret.data()[8] = fTxz - fTwy;
	ret.data()[9] = fTyz + fTwx;
	ret.data()[10] = 1.0f - ( fTxx + fTyy );
	ret.data()[15] = 1.0f;

	return ret;
}


Matrix Quaternion::inverseMatrix()
{
	Quaternion q = *this;
	q.w = -q.w;
	return q.matrix();
}

/*
Quaternion operator*( float im, const Quaternion& v )
{
	Quaternion ret;

	ret.x = v.x * im;
	ret.y = v.y * im;
	ret.z = v.z * im;
	ret.w = v.w * im;

	return ret;
}
*/
