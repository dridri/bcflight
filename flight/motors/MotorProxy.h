#ifndef MOTORPROXY_H
#define MOTORPROXY_H

#include "Motor.h"

class Main;
class Bus;

class MotorProxy : public Motor
{
public:
	MotorProxy( Bus* bus );
	~MotorProxy();

	virtual void Disarm();
	virtual void Disable();

	static Motor* Instanciate( Config* config, const string& object );
	static int flight_register( Main* main );

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update );

private:
	Bus* mBus;
};

#endif // MOTORPROXY_H
