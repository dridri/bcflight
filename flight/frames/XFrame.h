#ifndef XFRAME_H
#define XFRAME_H

#include <Vector.h>
#include "Frame.h"

class IMU;

class XFrame : public Frame
{
public:
	XFrame( Motor* fl, Motor* fr, Motor* rl, Motor* rr );
	virtual ~XFrame();

	void Arm();
	void Disarm();
	void WarmUp();
	void Stabilize( const Vector3f& pid_output, const float& thrust );

protected:
	float mStabSpeeds[4];
};

#endif // XFRAME_H
