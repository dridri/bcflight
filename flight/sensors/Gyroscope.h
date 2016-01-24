#ifndef GYROSCOPE_H
#define GYROSCOPE_H

#include <list>
#include <functional>
#include <string>
#include <Main.h>
#include "Sensor.h"

class Gyroscope : public Sensor
{
public:
	Gyroscope();
	~Gyroscope();

	const bool* axes() const;

	virtual void Read( Vector3f* v, bool raw = false ) = 0;

protected:
	bool mAxes[3];
};

#endif // GYROSCOPE_H
