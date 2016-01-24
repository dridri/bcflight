#ifndef EKFSMOOTHER_H
#define EKFSMOOTHER_H

#include <Matrix.h>

class EKFSmoother
{
public:
	EKFSmoother( int n, const Vector4f& q, const Vector4f& r );
	EKFSmoother( int n, float q, float r );
	~EKFSmoother();

	void setState( const Vector4f& v );
	void Predict( float dt );
	Vector4f Update( float dt, const Vector4f& vdata );

protected:
	int mDataCount;
	Matrix mQ;
	Matrix mR;
	Matrix mC;
	Matrix mSigma;
	Matrix mState;
};

#endif // EKFSMOOTHER_H
