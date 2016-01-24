#include "Gyroscope.h"

Gyroscope::Gyroscope()
	: Sensor()
	, mAxes{ false }
{
}


Gyroscope::~Gyroscope()
{
}


const bool* Gyroscope::axes() const
{
	return mAxes;
}
