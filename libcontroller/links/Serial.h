#ifndef SERIAL_H
#define SERIAL_H

#include "Link.h"

class Serial : public Link
{
public:
	Serial( const std::string& device, uint32_t baudrate = 115200 );
	~Serial();

	int Connect();
	void setFrequency( float f );
	int setBlocking( bool blocking );
	void setRetriesCount( int retries );
	int retriesCount() const;
	int32_t Channel();
	int32_t Frequency();
	int32_t RxQuality();
	int32_t RxLevel();
	uint32_t fullReadSpeed();

	int Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 );
	int Read( void* buf, uint32_t len, int32_t timeout );
	int32_t WriteAck( const void* buf, uint32_t len );
};

#endif // SERIAL_H
