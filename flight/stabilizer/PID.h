#ifndef PID_H
#define PID_H

#include "Vector.h"

class PID
{
public:
	PID();
	~PID();

	void Reset();

	void setP( float p );
	void setI( float i );
	void setD( float d );
	void setDeadBand( const Vector3f& band );

	void Process( const Vector3f& command, const Vector3f& measured, float dt );
	Vector3f state() const;

	Vector3f getPID() const;

private:
	Vector3f mIntegral;
	Vector3f mLastError;
	Vector3f mkPID;
	Vector3f mState;
	Vector3f mDeadBand;
};

#endif // PID_H
