#include "DShotDriver.h"


DShotDriver* DShotDriver::instance( bool create_new )
{
	return nullptr;
}


DShotDriver::DShotDriver()
{
}


DShotDriver::~DShotDriver()
{
}


void DShotDriver::Kill() noexcept
{
}


void DShotDriver::setPinValue( uint32_t pin, uint16_t value, bool telemetry )
{
	(void)pin;
	(void)value;
	(void)telemetry;
}


void DShotDriver::disablePinValue( uint32_t pin )
{
	(void)pin;
}


void DShotDriver::Update()
{
}
