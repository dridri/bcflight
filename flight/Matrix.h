#ifndef MATRIX_H
#define MATRIX_H

#include "Vector.h"

class Matrix
{
public:
	Matrix( int w = 4, int h = 4 );
	Matrix( const Matrix& other );
	virtual ~Matrix();

	float* data();
	const float* constData() const;
	const int width() const;
	const int height() const;

	void Clear();
	void Identity();
	void RotateX( float a );
	void RotateY( float a );
	void RotateZ( float a );
	Matrix Transpose();
	Matrix Inverse();

	void operator*=( const Matrix& other );
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
Vector4f operator*( const Matrix& m, const Vector4f& v );

#endif // MATRIX_H
