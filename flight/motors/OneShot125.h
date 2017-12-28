#ifndef ONESHOT125_H
#define ONESHOT125_H

#include <PWM.h>
#include "Motor.h"

class OneShot125 : public Motor
{
public:
	OneShot125( uint32_t pin, int us_min = 120, int us_max = 250 );
	~OneShot125();

	virtual void Disarm();
	virtual void Disable();

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update );
	PWM* mPWM;
	uint32_t mPin;
	uint32_t mMinUS;
	uint32_t mMaxUS;
};

#endif // ONESHOT125_H
