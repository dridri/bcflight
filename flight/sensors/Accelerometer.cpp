#include "Accelerometer.h"

Accelerometer::Accelerometer()
	: Sensor()
	, mAxes{ false }
{
}


Accelerometer::~Accelerometer()
{
}


const bool* Accelerometer::axes() const
{
	return mAxes;
}
