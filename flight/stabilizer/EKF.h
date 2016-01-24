#ifndef EKF_H
#define EKF_H

#include <Matrix.h>

class EKF
{
public:
	EKF();
	~EKF();

	void Predict( float dt, const Vector4f& data );
	Vector4f Update( float dt, const Vector4f& data1, const Vector4f& data2 );

protected:
	Matrix mQ;
	Matrix mR;
	Matrix mC;
	Matrix mSigma;
	Matrix mState;
};

#endif // EKF_H
