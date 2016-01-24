#ifndef MAGNETOMETER_H
#define MAGNETOMETER_H

#include <list>
#include <functional>
#include <string>
#include <Main.h>
#include "Sensor.h"

class Magnetometer : public Sensor
{
public:
	Magnetometer();
	~Magnetometer();

	const bool* axes() const;

	virtual void Read( Vector3f* v, bool raw = false ) = 0;

protected:
	bool mAxes[3];
};

#endif // MAGNETOMETER_H
