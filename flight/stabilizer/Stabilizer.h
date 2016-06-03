#ifndef STABILIZER_H
#define STABILIZER_H

#include <Frame.h>
#include "PID.h"

class Main;

class Stabilizer
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	Stabilizer( Main* main, Frame* frame );
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

	void setHorizonOffset( const Vector3f& v );
	Vector3f horizonOffset() const;

	void setMode( uint32_t mode );
	uint32_t mode() const;
	void setAltitudeHold( bool enabled );
	bool altitudeHold() const;

	void CalibrateESCs();
	void Reset( const float& yaw );
	void Update( IMU* imu, Controller* ctrl, float dt );

private:
	Frame* mFrame;
	Mode mMode;
	float mRateFactor;
	bool mAltitudeHold;

	PID mRatePID;
	PID mHorizonPID;
	PID mAltitudePID;
	float mAltitudeControl;

	int mLockState;
	Vector3f mHorizonMultiplier;
	Vector3f mHorizonOffset;
};

#endif // STABILIZER_H
