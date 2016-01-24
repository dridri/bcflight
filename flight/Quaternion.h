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
