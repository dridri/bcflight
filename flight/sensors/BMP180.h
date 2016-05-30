#ifndef BMP180_H
#define BMP180_H

#include <I2C.h>
#include "Altimeter.h"

class BMP180 : public Altimeter
{
public:
	BMP180();
	~BMP180();

	void Calibrate( float dt, bool last_pass = false );
	void Read( float* altitude );

	static Sensor* Instanciate();
	static int flight_register( Main* main );

private:
	I2C* mI2C;
};

#endif // BMP180_H
