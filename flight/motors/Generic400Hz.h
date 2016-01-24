#ifndef GENERIC400HZ_H
#define GENERIC400HZ_H

#include <Servo.h>
#include "Motor.h"

class Generic400Hz : public Motor
{
public:
	Generic400Hz( Servo* servo, float minspeed = 0.0f, float maxSpeed = 1.0f );
	~Generic400Hz();

	void Disarm();

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false );
	Servo* mServo;
	float mMinSpeed;
	float mMaxSpeed;
};

#endif // GENERIC400HZ_H
