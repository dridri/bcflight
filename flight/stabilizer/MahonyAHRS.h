#pragma once

#include "SensorFusion.h"
#include <Vector.h>
#include <Quaternion.h>
#include <Matrix.h>

class MahonyAHRS : public SensorFusion<Vector3f>
{
public:
	MahonyAHRS( float kp, float ki );
	virtual ~MahonyAHRS();

	virtual void UpdateInput( uint32_t row, const Vector3f& input );
	virtual void Process( float dt );
	virtual const Vector3f& state() const;

protected:
	void computeMatrix();

	Matrix mRotationMatrix;
	Quaternion mState;
	Vector3f mFinalState;
	Vector3f mIntegral;
	std::vector<Vector3f> mInputs;
	float mKp;
	float mKi;
};