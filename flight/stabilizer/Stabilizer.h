#ifndef STABILIZER_H
#define STABILIZER_H

#include <Frame.h>
#include <EKFSmoother.h>

class Stabilizer
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	Stabilizer( Frame* frame );
	~Stabilizer();

	void setP( float p );
	void setI( float i );
	void setD( float d );
	Vector3f getPID() const;
	Vector3f lastPIDOutput() const;

	void setOuterP( float p );
	void setOuterI( float i );
	void setOuterD( float d );
	Vector3f getOuterPID() const;
	Vector3f lastOuterPIDOutput() const;

	void setMode( uint32_t mode );
	uint32_t mode() const;

	void Reset( const float& yaw );
	void Update( IMU* imu, Controller* ctrl, float dt );

private:
	void CorrectDeadBand( Vector3f& error, const Vector3f& epsilon );
	Vector3f ProcessStabilize( IMU* imu, const Vector3f& ctrl_rpy, float dt );
	Vector3f ProcessRate( IMU* imu, const Vector3f& ctrl_rate, float dt );

	Frame* mFrame;
	Mode mMode;
	EKFSmoother mSmoothRate;

	Vector3f mIntegral;
	Vector3f mLastError;
	Vector3f mkPID;
	Vector3f mLastPID;

	Vector3f mTargetRPY;
	Vector3f mOuterIntegral;
	Vector3f mOuterLastError;
	Vector3f mOuterLastOutput;
	Vector3f mOuterkPID;
	Vector3f mLastOuterPID;
};

#endif // STABILIZER_H
