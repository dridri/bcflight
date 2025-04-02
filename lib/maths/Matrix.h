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

#ifndef MATRIX_H
#define MATRIX_H

#include <functional>
#include "Vector.h"

class Matrix
{
public:
	Matrix( int w = 4, int h = 4 );
	Matrix( const Matrix& other );
	Matrix( const vector<float>& vec, bool asColumn = true );
	virtual ~Matrix();

	void Orthogonal( float left, float right, float bottom, float top, float zNear, float zFar );

	float operator()( int x, int y ) const {
		return m[y * mWidth + x];
	}
	
	float& operator[]( int idx ) {
		return m[idx];
	}
	void applyFunction( std::function<float (float)> func ) {
		for ( int i = 0; i < mWidth * mHeight; i++ ) {
			m[i] = func(m[i]);
		}
	}

	float* data();
	const float* constData() const;
	const int width() const;
	const int height() const;
	const int size() const;

	void Clear();
	void Identity();
	void RotateX( float a );
	void RotateY( float a );
	void RotateZ( float a );
	Matrix Transpose();
	Matrix Inverse();

	void operator*=( const Matrix& other );
	void operator+=( const Matrix& other );
	void operator=( const Matrix& other );

// protected:
public:
	float* m;

protected:
	int mWidth;
	int mHeight;
};

Matrix operator+( const Matrix& m1, const Matrix& m2 );
Matrix operator-( const Matrix& m1, const Matrix& m2 );
Matrix operator*( const Matrix& m1, const Matrix& m2 );
Matrix operator*( const Matrix& m1, float v );
Vector4f operator*( const Matrix& m, const Vector4f& v );

#endif // MATRIX_H
