#include "Debug.h"
#include "Matrix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>

#define PI_OVER_360 0.0087266f
#define PI_OVER_180 0.0174532f
#define PI_OVER_90 0.0349065f

Matrix::Matrix( int w, int h )
	: mWidth( w )
	, mHeight( h )
{
	Identity();
}


Matrix::~Matrix()
{
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


void Matrix::Identity()
{
	memset( m, 0, sizeof( m ) );
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

	float c = std::cos( a );
	float s = std::sin( a );
	t.m[1*4+1] = c;
	t.m[1*4+2] = s;
	t.m[2*4+1] = -s;
	t.m[2*4+2] = c;

// 	*this *= t;
	operator*=(t);
}


void Matrix::RotateY( float a )
{
	Matrix t;
	t.Identity();

	float c = std::cos( a );
	float s = std::sin( a );
	t.m[0*4+0] = c;
	t.m[0*4+2] = -s;
	t.m[2*4+0] = s;
	t.m[2*4+2] = c;

// 	*this *= t;
	operator*=(t);
}


void Matrix::RotateZ( float a )
{
	Matrix t;
	t.Identity();

	float c = std::cos( a );
	float s = std::sin( a );
	t.m[0*4+0] = c;
	t.m[0*4+1] = s;
	t.m[1*4+0] = -s;
	t.m[1*4+1] = c;

// 	*this *= t;
	operator*=(t);
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
		gDebug() << "Error : cannort inverse a non-square matrix !\n";
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


void Matrix::operator=( const Matrix& other )
{
	mWidth = other.mWidth;
	mHeight = other.mHeight;
	memcpy( m, other.m, sizeof(float) * 16 );
}


Matrix operator+( const Matrix& m1, const Matrix& m2 )
{
	int w = std::min( m1.width(), m2.width() );
	int h = std::min( m1.height(), m2.height() );
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
	int w = std::min( m1.width(), m2.width() );
	int h = std::min( m1.height(), m2.height() );
	Matrix ret( w, h );

	for ( int j = 0; j < h; j++ ) {
		for ( int i = 0; i < w; i++ ) {
			ret.m[ j * w + i ] = m1.m[ j * m1.width() + i ] - m2.m[ j * m2.width() + i ];
		}
	}

	return ret;
}


Matrix operator*( const Matrix& m1, const Matrix& m2 )
{
	Matrix ret( m2.width(), m1.height() );
	memset( ret.m, 0, sizeof( ret.m ) );

	for ( int i = 0; i < ret.height(); i++ ) {
		for ( int j = 0; j < ret.width(); j++ ) {
			for ( int n = 0; n < m2.height(); n++ ) {
				ret.data()[ i * ret.width() + j ] += m1.constData()[ i * m1.width() + n ] * m2.constData()[ n * m2.width() + j ];
			}
		}
	}

	return ret;
}

void Matrix::operator*=( const Matrix& other )
{
	float ret[16];
	int i=0, j=0, k=0;

	for ( i = 0; i < 16; i++ ) {
		ret[i] = m[j] * other.m[k] + m[j+4] * other.m[k+1] + m[j+8] * other.m[k+2] + m[j+12] * other.m[k+3];
		k += 4 * ( ( j + 1 ) == 4 );
		j = ( j + 1 ) % 4;
	}

	memcpy( m, ret, sizeof(float) * 16 );
}


Vector4f operator*( const Matrix& m, const Vector4f& vec )
{
// 	fDebug0();
	Vector4f ret = Vector4f();

	for ( int j = 0; j < m.height(); j++ ) {
		for ( int i = 0; i < m.width(); i++ ) {
// 			printf( "ret[%d] += vec[%d] * m.constData()[ %d * %d + %d => %d ]\n", j, j, j, m.width(), i, j * m.width() + i );
			ret[j] += vec[i] * m.constData()[ j * m.width() + i ];
		}
	}

	return ret;
}

