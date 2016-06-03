#ifndef GPS_H
#define GPS_H

#include "Sensor.h"

class GPS : public Sensor
{
public:
	GPS();
	~GPS();

	virtual void Read( float* altitude ) = 0;
};

#endif // GPS_H
