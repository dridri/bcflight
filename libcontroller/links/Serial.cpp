#include "Serial.h"

Serial::Serial( const std::string& device, uint32_t baudrate )
{
}


Serial::~Serial()
{
}


int Serial::setBlocking( bool blocking )
{
	(void)blocking;
	return 0;
}


void Serial::setRetriesCount( int retries )
{
}


int Serial::retriesCount() const
{
	return 0;
}


int32_t Serial::Channel()
{
	return 0;
}


int32_t Serial::Frequency()
{
	return 0;
}


int32_t Serial::RxQuality()
{
	return 0;
}


int32_t Serial::RxLevel()
{
	return 0;
}


int Serial::Connect()
{
	return 0;
}


int Serial::Read( void* buf, uint32_t len, int32_t timeout )
{
	return 0;
}


int Serial::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
	return 0;
}


int32_t Serial::WriteAck( const void* buf, uint32_t len )
{
	return 0;
}
