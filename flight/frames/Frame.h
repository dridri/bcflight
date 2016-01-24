#ifndef FRAME_H
#define FRAME_H

#include <vector>
#include <stdint.h>
#include <Motor.h>
#include <Vector.h>

class IMU;
class Controller;

class Frame
{
public:
	Frame();
	virtual ~Frame();

	const std::vector< Motor* >& motors() const;

	virtual void Arm() = 0;
	virtual void Disarm() = 0;
	virtual void WarmUp() = 0;
	virtual void Stabilize( const Vector3f& pid_output, const float& thrust ) = 0;

protected:
	std::vector< Motor* > mMotors;
};

#endif // FRAME_H
