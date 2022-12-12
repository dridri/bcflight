#ifndef DSHOT_H
#define DSHOT_H

#include <PWM.h>
#include "Motor.h"

class DShotDriver;

LUA_CLASS class DShot : public Motor
{
public:
	LUA_EXPORT DShot( uint32_t pin );
	~DShot();

	virtual void Disarm();
	virtual void Disable();

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update );
	DShotDriver* mDriver;
	uint32_t mPin;
};

#endif // DSHOT_H
