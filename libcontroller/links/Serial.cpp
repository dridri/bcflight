#include "Serial.h"

Serial::Serial( const std::string& device, uint32_t baudrate )
{
}


Serial::~Serial()
{
}


int Serial::setBlocking( bool blocking )
{
}


void Serial::setRetriesCount( int retries )
{
}


int Serial::retriesCount() const
{
}


int32_t Serial::Channel()
{
}


int32_t Serial::Frequency()
{
}


int32_t Serial::RxQuality()
{
}


int32_t Serial::RxLevel()
{
}


int Serial::Connect()
{
}


int Serial::Read( void* buf, uint32_t len, int32_t timeout )
{
}


int Serial::Write( const void* buf, uint32_t len, bool ack, int32_t timeout )
{
}


int32_t Serial::WriteAck( const void* buf, uint32_t len )
{
}
