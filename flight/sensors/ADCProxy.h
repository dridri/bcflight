#ifndef ADCPROXY_H
#define ADCPROXY_H

#include "Voltmeter.h"

class ADCProxy : public Voltmeter
{
public:
	ADCProxy();
	~ADCProxy();

	static int flight_register( Main* main );
	static Sensor* Instanciate( Config* config, const string& object, Bus* bus );

	void Calibrate( float dt, bool last_pass = false );
	float Read( int channel );

protected:
};

#endif // ADCPROXY_H
