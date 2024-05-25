#ifndef ONESHOT125_H
#define ONESHOT125_H

#include <PWM.h>
#include <Lua.h>
#include "Motor.h"

class Main;

LUA_CLASS class OneShot125 : public Motor
{
public:
	OneShot125( uint32_t pin, int us_min = 125, int us_max = 250-8, PWM* pwm = nullptr );
	LUA_EXPORT OneShot125( const LuaValue& pin );
	~OneShot125();

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
	uint32_t mScale;
};

#endif // ONESHOT125_H
