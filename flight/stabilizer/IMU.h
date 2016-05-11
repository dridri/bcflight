#ifndef IMU_H
#define IMU_H

#include <Main.h>
#include <Thread.h>
#include <Vector.h>
#include <EKF.h>
#include <EKFSmoother.h>

#define IMU_RPY_SMOOTH_RATIO 0.02f

class IMU
{
public:
	typedef enum {
		Off,
		Calibrating,
		CalibratingAll,
		CalibrationDone,
		Running
	} State;

	IMU( Main* main );
	virtual ~IMU();

	const Vector3f acceleration() const;
	const Vector3f gyroscope() const;
	const Vector3f magnetometer() const;

	const State& state() const;
	const Vector3f RPY() const;
	const Vector3f dRPY() const;
	const Vector3f rate() const;

	void Recalibrate();
	void RecalibrateAll();
	void ResetRPY();
	void ResetYaw();
	void Loop( float dt );

protected:
	bool SensorsThreadRun();
	void Calibrate( float dt, bool all = false );
	void UpdateSensors( float dt, bool gyro_only = false );
	void UpdateRPY( float dt );

	Main* mMain;
	HookThread<IMU>* mSensorsThread;
	uint64_t mSensorsThreadTick;
	uint64_t mSensorsThreadTickRate;

	// Running states
	State mState;
	Vector3f mAcceleration;
	Vector3f mGyroscope;
	Vector3f mMagnetometer;
	Vector3f mRPY;
	Vector3f mdRPY;
	Vector3f mRate;
	Vector3f mRPYOffset;
	Vector4f mAccelerometerOffset;

	// Calibration states
	int mCalibrationAccum;
	Vector4f mRPYAccum;
	Vector4f mdRPYAccum;

	EKF mRates;
	EKF mAttitude;
	Vector4f mLastAccelAttitude;
	Vector3f mVirtualNorth;

	Vector3f mLastAcceleration;
	uint32_t mAcroRPYCounter;
};

#endif // IMU_H
