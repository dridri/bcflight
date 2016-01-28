#ifndef CURRENTSENSOR_H
#define CURRENTSENSOR_H

#include <list>
#include <functional>
#include <string>
#include <Main.h>
#include "Sensor.h"

class CurrentSensor : public Sensor
{
public:
	CurrentSensor();
	~CurrentSensor();

	virtual float Read( int channel = 0 ) = 0;

protected:
};

#endif // CURRENTSENSOR_H
