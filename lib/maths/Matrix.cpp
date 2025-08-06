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
#include "Debug.h"
#include "Matrix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>

#define PI_OVER_360 0.0087266f
#define PI_OVER_180 0.0174532f
#define PI_OVER_90 0.0349065f

#define min( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define max( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

Matrix::Matrix( int w, int h )
	: mWidth( w )
	, mHeight( h )
{
	m = (float*)malloc( sizeof(float) * w * h );
	Identity();
}


Matrix::Matrix( const Matrix& other )
	: mWidth( other.mWidth )
	, mHeight( other.mHeight )
{
	m = (float*)malloc( sizeof(float) * mWidth * mHeight );
	memcpy( m, other.m, sizeof(float) * mWidth * mHeight );
}


Matrix::Matrix( const vector<float>& vec, bool asColumn ) {
	mWidth = ( asColumn ? 1 : vec.size() );
	mHeight = ( asColumn ? 1 : vec.size() );
	m = (float*)malloc( sizeof(float) * mWidth * mHeight );
	memcpy( m, vec.data(), sizeof(float) * vec.size() );
}


Matrix::~Matrix()
{
	if ( m == nullptr ) {
		gDebug() << "CRITICAL : corrupt matrix !!!";
		exit(0);
	}
	free( m );
	m = nullptr;
}


void Matrix::Orthogonal( float left, float right, float bottom, float top, float zNear, float zFar )
{
	Identity();

	float tx = - (right + left) / (right - left);
	float ty = - (top + bottom) / (top - bottom);
	float tz = - (zFar + zNear) / (zFar - zNear);

	m[0] = 2.0 / (right - left);
	m[5] = 2.0 / (top - bottom);
	m[10] = -2.0 / (zFar - zNear);

	m[12] = tx;
	m[13] = ty;
	m[14] = tz;
}


float* Matrix::data()
{
	return m;
}


const float* Matrix::constData() const
{
	return (float*)m;
}


const int Matrix::width() const
{
	return mWidth;
}


const int Matrix::height() const
{
	return mHeight;
}


const int Matrix::size() const
{
	return mWidth * mHeight;
}


void Matrix::Clear()
{
	if ( m == nullptr ) {
		gDebug() << "CRITICAL : corrupt matrix !!!";
		exit(0);
	}
	memset( m, 0, sizeof(float) * mWidth * mHeight );
}


void Matrix::Identity()
{
	Clear();
	if ( mWidth == mHeight ) {
		for ( int i = 0; i < mWidth * mHeight; i++ ) {
			if ( i % ( mWidth + 1 ) == 0 ) {
				m[i] = 1.0f;
			}
		}
	}
}


void Matrix::RotateX( float a )
{
	Matrix t;
	t.Identity();

	float c = cos( a );
	float s = sin( a );
	t.m[1*4+1] = c;
	t.m[1*4+2] = s;
	t.m[2*4+1] = -s;
	t.m[2*4+2] = c;

// 	operator*=(t);
}


void Matrix::RotateY( float a )
{
	Matrix t;
	t.Identity();

	float c = cos( a );
	float s = sin( a );
	t.m[0*4+0] = c;
	t.m[0*4+2] = -s;
	t.m[2*4+0] = s;
	t.m[2*4+2] = c;

// 	operator*=(t);
}


void Matrix::RotateZ( float a )
{
	Matrix t;
	t.Identity();

	float c = cos( a );
	float s = sin( a );
	t.m[0*4+0] = c;
	t.m[0*4+1] = s;
	t.m[1*4+0] = -s;
	t.m[1*4+1] = c;

// 	operator*=(t);
}


Matrix Matrix::Transpose()
{
	Matrix ret( mHeight, mWidth );

	for ( int j = 0; j < mHeight; j++ ) {
		for ( int i = 0; i < mWidth; i++ ) {
			ret.m[ i * mHeight + j ] = m[ j * mWidth + i ];
		}
	}

	return ret;
}


Matrix Matrix::Inverse()
{
	if ( mWidth != mHeight ) {
		gDebug() << "Error : cannort inverse a non-square matrix !";
		return *this;
	}

	Matrix tmp( mWidth, mHeight );
	Matrix ret( mWidth, mHeight );
	int i, I;
	int k, K;

	for ( i = 0; i < mWidth*mWidth; i++ ) {
		tmp.m[i] = m[i];
	}

	for ( i = 0, I = 0; i < mWidth; i++, I += mWidth ) {
		ret.m[ I + i ] = 1.0 / tmp.m[ I + i ];
		for ( int j = 0; j < mWidth; j++ ) {
			if ( j != i ) {
				ret.m[ I + j ] = -tmp.m[ I + j ] / tmp.m[ I + i ];
			}

			for ( k = 0, K = 0; k < mWidth; k++, K += mWidth ) {
				if ( k != i ) {
					ret.m[ K + i ] = tmp.m[ K + i ] / tmp.m[ I + i ];
				}
				if ( j != i && k != i ) {
					ret.m[ K + j ] = tmp.m[ K + j ] - tmp.m[ I + j ] * tmp.m[ K + i ] / tmp.m[ I + i ];
				}
			}
		}

		for ( k = 0; k < mWidth*mWidth; k++ ) {
			tmp.m[k] = ret.m[k];
		}
	}

	return ret;
}


void Matrix::operator+=( const Matrix& other )
{
	Matrix r = *this + other;
	memcpy( m, r.m, sizeof(float) * r.size() );
}


void Matrix::operator=( const Matrix& other )
{
	if ( other.mWidth == mWidth and other.mHeight == mHeight ) {
		memcpy( m, other.m, sizeof(float) * mWidth * mHeight );
		return;
	}

	mWidth = other.mWidth;
	mHeight = other.mHeight;
	if ( m ) {
		free( m );
	}
	m = (float*)malloc( sizeof(float) * mWidth * mHeight );
	memcpy( m, other.m, sizeof(float) * mWidth * mHeight );
}


Matrix operator+( const Matrix& m1, const Matrix& m2 )
{
	int w = min( m1.width(), m2.width() );
	int h = min( m1.height(), m2.height() );
	Matrix ret( w, h );

	for ( int j = 0; j < h; j++ ) {
		for ( int i = 0; i < w; i++ ) {
			ret.m[ j * w + i ] = m1.m[ j * m1.width() + i ] + m2.m[ j * m2.width() + i ];
		}
	}

	return ret;
}


Matrix operator-( const Matrix& m1, const Matrix& m2 )
{
	int w = min( m1.width(), m2.width() );
	int h = min( m1.height(), m2.height() );
	Matrix ret( w, h );

	for ( int j = 0, j1 = 0, j2 = 0; j < h; j++, j1 += m1.width(), j2 += m2.width() ) {
		for ( int i = 0; i < w; i++ ) {
			ret.m[ j * w + i ] = m1.m[ j * m1.width() + i ] - m2.m[ j * m2.width() + i ];
// 			ret.m[ j * w + i ] = m1.m[ j1 + i ] - m2.m[ j2 + i ];
		}
	}

	return ret;
}

/*
#if ( defined(__ARM_FP) && defined(__ARM_NEON) )
#include <arm_neon.h>
void matrix_multiply_4x4_neon( float32_t* C, float32_t* A, float32_t* B ) {
	float32x4_t A0, A1, A2, A3;
	float32x4_t B0, B1, B2, B3;
	float32x4_t C0, C1, C2, C3;

	A0 = vld1q_f32( A );
	A1 = vld1q_f32( A + 4 );
	A2 = vld1q_f32( A + 8 );
	A3 = vld1q_f32( A + 12 );

	B0 = vld1q_f32( B );
	B1 = vld1q_f32( B + 4 );
	B2 = vld1q_f32( B + 8 );
	B3 = vld1q_f32( B + 12 );

	C0 = vmovq_n_f32( 0 );
	C1 = vmovq_n_f32( 0 );
	C2 = vmovq_n_f32( 0 );
	C3 = vmovq_n_f32( 0 );

	C0 = vfmaq_laneq_f32( C0, A0, B0, 0 );
	C0 = vfmaq_laneq_f32( C0, A1, B0, 1 );
	C0 = vfmaq_laneq_f32( C0, A2, B0, 2 );
	C0 = vfmaq_laneq_f32( C0, A3, B0, 3 );
	vst1q_f32(C, C0 );

	C1 = vfmaq_laneq_f32( C1, A0, B1, 0 );
	C1 = vfmaq_laneq_f32( C1, A1, B1, 1 );
	C1 = vfmaq_laneq_f32( C1, A2, B1, 2 );
	C1 = vfmaq_laneq_f32( C1, A3, B1, 3 );
	vst1q_f32( C + 4, C1 );

	C2 = vfmaq_laneq_f32( C2, A0, B2, 0 );
	C2 = vfmaq_laneq_f32( C2, A1, B2, 1 );
	C2 = vfmaq_laneq_f32( C2, A2, B2, 2 );
	C2 = vfmaq_laneq_f32( C2, A3, B2, 3 );
	vst1q_f32( C + 8, C2 );

	C3 = vfmaq_laneq_f32( C3, A0, B3, 0 );
	C3 = vfmaq_laneq_f32( C3, A1, B3, 1 );
	C3 = vfmaq_laneq_f32( C3, A2, B3, 2 );
	C3 = vfmaq_laneq_f32( C3, A3, B3, 3 );
	vst1q_f32( C + 12, C3 );
}
#endif
*/

Matrix operator*( const Matrix& m1, const Matrix& m2 )
{
	Matrix ret( m2.width(), m1.height() );
	memset( ret.data(), 0, sizeof(float) * ret.width() * ret.height() );
/*
#if ( defined(__ARM_FP) && defined(__ARM_NEON) )
	if ( m1.width() == 4 and m1.height() == 4 and m2.width() == 4 and m2.height() == 4 ) {
		matrix_multiply_4x4_neon( ret.data(), m1.data(), m2.data() );
		return ret;
	}
#endif
*/
	for ( int i = 0; i < ret.height(); i++ ) {
		for ( int j = 0; j < ret.width(); j++ ) {
			for ( int n = 0; n < m2.height(); n++ ) {
				ret.data()[ i * ret.width() + j ] += m1.constData()[ i * m1.width() + n ] * m2.constData()[ n * m2.width() + j ];
			}
		}
	}

	return ret;
}


Vector4f operator*( const Matrix& m, const Vector4f& vec )
{
	Vector4f ret = Vector4f();

	for ( int j = 0, line = 0; j < m.height(); j++, line += m.width() ) {
		for ( int i = 0; i < m.width(); i++ ) {
			ret[j] += vec[i] * m.constData()[ j * m.width() + i ];
		}
	}

	return ret;
}


Matrix operator*( const Matrix& m1, float v )
{
	Matrix ret( m1.width(), m1.height() );

	for ( int i = 0; i < ret.size(); i++ ) {
		ret.data()[i] = m1.constData()[i] * v;
	}

	return ret;
}
