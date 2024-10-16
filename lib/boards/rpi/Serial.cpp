#include "Serial.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
// #include <sys/ioctl.h>
// #include <termios.h>
#include <asm/termios.h>
#include <map>
#include "Debug.h"
#include <poll.h>


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


Serial::Serial( const string& device, int speed, int read_timeout )
	: Bus()
	, mFD( -1 )
	, mOptions( nullptr )
	, mDevice( device )
	, mSpeed( speed )
	, mReadTimeout( read_timeout )
{
}


Serial::~Serial()
{
	delete mOptions;
}


int Serial::Connect()
{

	mFD = open( mDevice.c_str(), O_RDWR | O_NOCTTY );
	if ( mFD < 0 ) {
		gError() << "Err0 : " << strerror(errno);
		return -1;
	}

	mOptions = new struct termios2;
	ioctl( mFD, TCGETS2, mOptions );

	// Set stop bits to 1
	mOptions->c_cflag &= ~CSTOPB;

	// Set parity options
	mOptions->c_cflag &= ~PARENB; // Disable parity
	mOptions->c_cflag &= ~PARODD; // Even parity

	mOptions->c_cflag |= CRTSCTS;

	// Set data bits
	mOptions->c_cflag &= ~CSIZE;
	mOptions->c_cflag |= CS8; // 8 data bits


	// Enable binary mode
	mOptions->c_iflag = 0;
	mOptions->c_oflag &= ~OPOST;
	mOptions->c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	mOptions->c_cc[VMIN] = 6;
	mOptions->c_cc[VTIME] = 0;


	mOptions->c_cflag &= ~CBAUD;
	mOptions->c_cflag |= BOTHER;

	mOptions->c_ispeed = mSpeed;
	mOptions->c_ospeed = mSpeed;

	int ret = ioctl( mFD, TCSETS2, mOptions );
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


void Serial::setStopBits( uint8_t count )
{
	if ( not mOptions ) {
		return;
	}

	ioctl( mFD, TCGETS2, mOptions );

	if ( count == 2 ) {
		mOptions->c_cflag |= CSTOPB;
	} else if ( count == 1 ) {
		mOptions->c_cflag &= ~CSTOPB;
	} else {
		gWarning() << "Unhandled stop bits count (" << count << ")";
	}

	int ret = ioctl( mFD, TCSETS2, mOptions );
	if ( ret < 0 ) {
		gError() << "Err1 : " << errno << ", " << strerror(errno);
	}
}


int Serial::Read( uint8_t reg, void* buf, uint32_t len )
{
	return 0;
}


int Serial::Read( void* buf, uint32_t len )
{
	if ( mReadTimeout > 0 ) {
		struct pollfd pfd[1];
		pfd[0].fd = mFD;
		pfd[0].events = POLLIN;
		int ret = poll( pfd, 1, mReadTimeout );
		if ( ret <= 0 ) {
			return ret;
		}
	}

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
	return ret;
}


int Serial::Write( uint8_t reg, const void* buf, uint32_t len )
{
	return 0;
}
