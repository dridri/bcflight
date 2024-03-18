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


Serial::Serial( const string& device, int speed, bool singleWire )
	: Bus()
	, mFD( -1 )
	, mSingleWire( singleWire )
	, mDevice( device )
	, mSpeed( speed )
{
}


Serial::~Serial()
{
}


int Serial::Connect()
{
	mFD = open( mDevice.c_str(), O_RDWR | O_NOCTTY/* | O_NDELAY*/ );
	if ( mFD < 0 ) {
		gError() << "Err0 : " << strerror(errno);
		return -1;
	}

	struct termios2 options;

	ioctl( mFD, TCGETS2, &options );
	
	// Set baud rate
	options.c_cflag &= ~CBAUD;
	options.c_cflag |= B115200; // Example baud rate: 115200

	// Set data bits
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8; // 8 data bits

	// Disable parity
	options.c_cflag &= ~PARENB;

	// Set stop bits
	options.c_cflag &= ~CSTOPB; // 1 stop bit

	// Disable hardware flow control
	options.c_cflag &= ~CRTSCTS;


    // Enable binary mode
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR);
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_cc[VMIN] = 1; // 25
    options.c_cc[VTIME] = 0;

    options.c_cflag &= ~CBAUD;
    options.c_cflag |= BOTHER;

	options.c_ispeed = mSpeed;
	options.c_ospeed = mSpeed;
	int ret = ioctl( mFD, TCSETS2, &options );
	if ( ret < 0 ) {
		gError() << "Err1 : " << errno << ", " << strerror(errno);
		return -1;
	}

	return 0;
}


std::string Serial::toString()
{
	return mDevice;
}



int Serial::Read( uint8_t reg, void* buf, uint32_t len )
{
	return 0;
}


int Serial::Read( void* buf, uint32_t len )
{
	int ret = read( mFD, buf, len );
	if ( ret < 0 && errno != EAGAIN ) {
		gDebug() << errno << ", " << strerror(errno);
	}
	return ret;
}


int Serial::Write( const void* buf, uint32_t len )
{
	int ret = write( mFD, buf, len );
	if ( ret < 0 ) {
		gDebug() << errno << ", " << strerror(errno);
	}
	if ( mSingleWire ) {
		usleep( 1000 );
		void* dummy = new uint8_t[ len ];
		ret = read( mFD, dummy, len );
		delete[] dummy;
		if ( ret < 0 ) {
			gDebug() << "Single-wire echo-read failed: " << errno << ", " << strerror(errno);
		}
		if ( ret != len ) {
			gDebug() << "Single-wire echo-read failed: " << ret << " != " << len;
		}
	}
	return ret;
}


int Serial::Write( uint8_t reg, const void* buf, uint32_t len )
{
	return 0;
}
