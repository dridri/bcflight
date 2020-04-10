#ifndef DSHOT600_H
#define DSHOT600_H

#include <PWM.h>
#include "Motor.h"

class Main;

class DShot : public Motor
{
public:
	DShot( uint32_t pin );
	~DShot();

	virtual void Disarm();
	virtual void Disable();

	static Motor* Instanciate( Config* config, const string& object );
	static int flight_register( Main* main );

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update );
	PWM* mPWM;
	uint8_t mBuffer[14];
};

#endif // DSHOT600_H
