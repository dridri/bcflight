#include "Serial.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
// #include <sys/ioctl.h>
// #include <termios.h>
#include <asm/termios.h>
#include <map>
#include "Debug.h"

//#include <pigpio.h>

extern "C" int ioctl (int __fd, unsigned long int __request, ...) __THROW;

static map< int, int > sSpeeds = {
	{ 0, B0 },
	{ 50, B50 },
	{ 75, B75 },
	{ 110, B110 },
	{ 134, B134 },
	{ 150, B150 },
	{ 200, B200 },
	{ 300, B300 },
	{ 600, B600 },
	{ 1200, B1200 },
	{ 1800, B1800 },
	{ 2400, B2400 },
	{ 4800, B4800 },
	{ 9600, B9600 },
	{ 19200, B19200 },
	{ 38400, B38400 },
	{ 57600, B57600 },
	{ 115200, B115200 },
	{ 230400, B230400 },
	{ 460800, B460800 },
	{ 500000, B500000 },
	{ 576000, B576000 },
	{ 921600, B921600 },
	{ 1000000, B1000000 },
	{ 1152000, B1152000 },
	{ 1500000, B1500000 },
	{ 2000000, B2000000 },
	{ 2500000, B2500000 },
	{ 3000000, B3000000 },
	{ 3500000, B3500000 },
	{ 4000000, B4000000 }
};


Serial::Serial( const string& device, int speed )
	: Bus()
	, mFD( -1 )
	, mDevice( device )
{
}


Serial::~Serial()
{
}


int Serial::Connect()
{
	return 0;
}


std::string Serial::toString()
{
	return mDevice;
}


void Serial::setStopBits( uint8_t count )
{
	(void)count;
}


int Serial::Read( uint8_t reg, void* buf, uint32_t len )
{
	return 0;
}


int Serial::Read( void* buf, uint32_t len )
{
	return 0;
}


int Serial::Write( const void* buf, uint32_t len )
{
	return 0;
}


int Serial::Write( uint8_t reg, const void* buf, uint32_t len )
{
	return 0;
}
