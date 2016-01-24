#include "Magnetometer.h"

Magnetometer::Magnetometer()
	: Sensor()
	, mAxes{ false }
{
}


Magnetometer::~Magnetometer()
{
}


const bool* Magnetometer::axes() const
{
	return mAxes;
}
