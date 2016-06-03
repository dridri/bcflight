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

	static Sensor* Instanciate( Config* config, const std::string& object );
	static int flight_register( Main* main );

private:
	float ReadTemperature();
	float ReadPressure();

	I2C* mI2C;
	float mBasePressure;

	int16_t AC1, AC2, AC3, VB1, VB2, MB, MC, MD;
	uint16_t AC4, AC5, AC6; 
	double c5, c6, mc, md, x0, x1, x2, y0, y1, y2, p0, p1, p2;
	char _error;
};

#endif // BMP180_H
