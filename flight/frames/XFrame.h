#ifndef XFRAME_H
#define XFRAME_H

#include <Vector.h>
#include "Frame.h"

class IMU;

class XFrame : public Frame
{
public:
	XFrame( Config* config );
	virtual ~XFrame();

	void Arm();
	void Disarm();
	void WarmUp();
	virtual bool Stabilize( const Vector3f& pid_output, const float& thrust );

	static Frame* Instanciate( Config* config );
	static int flight_register( Main* main );

protected:
	float mStabSpeeds[4];
	Vector3f mPIDMultipliers[4];
	float mMaxSpeed;
	bool mAirMode;
};

#endif // XFRAME_H
