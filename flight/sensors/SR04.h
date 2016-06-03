#ifndef SR04_H
#define SR04_H

#include "Altimeter.h"

class SR04 : public Altimeter
{
public:
	SR04( uint32_t gpio_trigger, uint32_t gpio_echo );
	~SR04();

	virtual Type type() const { return Proximity; }
	virtual void Calibrate( float dt, bool last_pass );
	virtual void Read( float* altitude );

	virtual std::string infos();

	static Sensor* Instanciate( Config* config, const std::string& object );
	static int flight_register( Main* main );

protected:
	uint32_t mTriggerPin;
	uint32_t mEchoPin;
	float mAltitude;
};

#endif // SR04_H
