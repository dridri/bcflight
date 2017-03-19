#ifndef DSHOT600_H
#define DSHOT600_H

#include <PWM.h>
#include "Motor.h"

class DShot600 : public Motor
{
public:
	DShot600( uint32_t pin );
	~DShot600();

	virtual void Disarm();
	virtual void Disable();

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update );
	PWM* mPWM;
	uint8_t mBuffer[14];
};

#endif // DSHOT600_H
