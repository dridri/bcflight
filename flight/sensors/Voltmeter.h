#ifndef VOLTMETER_H
#define VOLTMETER_H

#include <list>
#include <functional>
#include <string>
#include <Main.h>
#include "Sensor.h"

class Voltmeter : public Sensor
{
public:
	Voltmeter();
	~Voltmeter();

	virtual float Read( int channel = 0 ) = 0;

protected:
};

#endif // VOLTMETER_H
