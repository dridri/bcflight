#ifndef GENERIC_H
#define GENERIC_H

#include <Servo.h>
#include "Motor.h"

class Generic : public Motor
{
public:
	Generic( Servo* servo, float minspeed = 0.0f, float maxSpeed = 1.0f );
	~Generic();

	void Disarm();

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false );
	Servo* mServo;
	float mMinSpeed;
	float mMaxSpeed;
};

#endif // GENERIC_H
