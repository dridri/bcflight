#ifndef ONESHOT42_H
#define ONESHOT42_H

#include <PWM.h>
#include "Motor.h"

class Main;

class OneShot42 : public Motor
{
public:
	OneShot42( uint32_t pin, int us_min = 42, int us_max = 84-4 );
	~OneShot42();

	virtual void Disarm();
	virtual void Disable();

	static Motor* Instanciate( Config* config, const string& object );
	static int flight_register( Main* main );

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update );
	PWM* mPWM;
	uint32_t mPin;
	uint32_t mMinUS;
	uint32_t mMaxUS;
	uint32_t mMinValue;
	float mScale;
};

#endif // ONESHOT42_H
