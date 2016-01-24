#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <list>
#include <functional>
#include <string>
#include <Main.h>
#include "Sensor.h"

class Accelerometer : public Sensor
{
public:
	Accelerometer();
	~Accelerometer();

	const bool* axes() const;

	virtual void Read( Vector3f* v, bool raw = false ) = 0;

protected:
	bool mAxes[3];
};

#endif // ACCELEROMETER_H
