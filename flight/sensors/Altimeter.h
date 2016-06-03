#ifndef ALTIMETER_H
#define ALTIMETER_H

#include "Sensor.h"

class Altimeter : public Sensor
{
public:
	typedef enum {
		Absolute,
		Proximity
	} Type;

	Altimeter();
	~Altimeter();

	virtual Type type() const { return Absolute; }
	virtual void Read( float* altitude ) = 0;
};

#endif // ALTIMETER_H
