#include "ADCProxy.h"

extern uint16_t _g_ADCChan1; // Ugly HACK

int ADCProxy::flight_register( Main* main )
{
	Device dev;
	dev.name = "ADCProxy";
	dev.fInstanciate = ADCProxy::Instanciate;
	mKnownDevices.push_back( dev );
	return 0;
}


Sensor* ADCProxy::Instanciate( Config* config, const string& object, Bus* bus )
{
	// TODO : use bus
	return new ADCProxy();
}

ADCProxy::ADCProxy()
	: Voltmeter()
{
}


ADCProxy::~ADCProxy()
{
}


void ADCProxy::Calibrate( float dt, bool last_pass )
{
}


float ADCProxy::Read( int channel )
{
	if ( channel == 0 ) {
		return ((float)_g_ADCChan1) * 5.0f / 4096.0f;
	}
	return 0.0f;
}
