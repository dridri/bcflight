#ifndef ALTIMETER_H
#define ALTIMETER_H

#include "Sensor.h"

class Altimeter : public Sensor
{
public:
	Altimeter();
	~Altimeter();

	virtual void Read( float* altitude ) = 0;
};

#endif // ALTIMETER_H
