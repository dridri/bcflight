#include "spi.h"
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <iostream>

static std::string mSPIDevice = "";
static pthread_mutex_t spiMutex = PTHREAD_MUTEX_INITIALIZER;

static int mFD = -1;
static int mBitsPerWord = 8;
static struct spi_ioc_transfer mXFer[10];

nRF24::SPI::SPI()
{
}


nRF24::SPI::~SPI()
{
}


void nRF24::SPI::begin( const std::string& device )
{
	mSPIDevice = device;
}


void nRF24::SPI::end()
{
}


void nRF24::SPI::beginTransaction( SPISettings settings )
{
	if ( mFD < 0 ) {
		uint32_t speed = 0;
		switch ( settings.clck ) {
			case BCM2835_SPI_SPEED_64MHZ:
				speed = 64000000;
				break;
			case BCM2835_SPI_SPEED_32MHZ:
				speed = 32000000;
				break;
			case BCM2835_SPI_SPEED_16MHZ:
				speed = 16000000;
				break;
			case BCM2835_SPI_SPEED_8MHZ:
				speed = 8000000;
				break;
			case BCM2835_SPI_SPEED_4MHZ:
				speed = 4000000;
				break;
			case BCM2835_SPI_SPEED_2MHZ:
				speed = 2000000;
				break;
			case BCM2835_SPI_SPEED_1MHZ:
				speed = 1000000;
				break;
			case BCM2835_SPI_SPEED_512KHZ:
				speed = 512000;
				break;
			case BCM2835_SPI_SPEED_256KHZ:
				speed = 256000;
				break;
			case BCM2835_SPI_SPEED_128KHZ:
				speed = 128000;
				break;
			case BCM2835_SPI_SPEED_64KHZ:
				speed = 64000;
				break;
			case BCM2835_SPI_SPEED_32KHZ:
				speed = 32000;
				break;
			case BCM2835_SPI_SPEED_16KHZ:
				speed = 16000;
				break;
			case BCM2835_SPI_SPEED_8KHZ:
			default:
				speed = 8000;
				break;
		}
		mFD = open( mSPIDevice.c_str(), O_RDWR );
		std::cout << "SPI fd (" << mSPIDevice << ") : " << mFD << "\n";

		uint8_t mode, lsb=0;

		mode = SPI_MODE_0;

		if ( ioctl( mFD, SPI_IOC_WR_MODE, &mode ) < 0 )
		{
			std::cout << "SPI wr_mode\n";
		}
		if ( ioctl( mFD, SPI_IOC_RD_MODE, &mode ) < 0 )
		{
			std::cout << "SPI rd_mode\n";
		}
		if ( ioctl( mFD, SPI_IOC_WR_BITS_PER_WORD, &mBitsPerWord ) < 0 ) 
		{
			std::cout << "SPI write bits_per_word\n";
		}
		if ( ioctl( mFD, SPI_IOC_RD_BITS_PER_WORD, &mBitsPerWord ) < 0 ) 
		{
			std::cout << "SPI read bits_per_word\n";
		}
		if ( ioctl( mFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed ) < 0 )  
		{
			std::cout << "can't set max speed hz\n";
		}

		if ( ioctl( mFD, SPI_IOC_RD_MAX_SPEED_HZ, &speed ) < 0 ) 
		{
			std::cout << "SPI max_speed_hz\n";
		}
		if ( ioctl( mFD, SPI_IOC_RD_LSB_FIRST, &lsb ) < 0 )
		{
			std::cout << "SPI rd_lsb_fist\n";
		}

		std::cout << "SPI " << mSPIDevice.c_str() << ":" << mFD <<": spi mode " << (int)mode << ", " << mBitsPerWord << "bits " << ( lsb ? "(lsb first) " : "" ) << "per word, " << speed << " Hz max\n";

		memset( mXFer, 0, sizeof( mXFer ) );
		for ( uint32_t i = 0; i < sizeof( mXFer ) /  sizeof( struct spi_ioc_transfer ); i++) {
			mXFer[i].len = 0;
			mXFer[i].cs_change = 0; // Keep CS activated
			mXFer[i].delay_usecs = 0;
			mXFer[i].speed_hz = speed;
			mXFer[i].bits_per_word = 8;
		}
	}

	pthread_mutex_lock( &spiMutex );
}


void nRF24::SPI::endTransaction()
{
	pthread_mutex_unlock( &spiMutex );
}


void nRF24::SPI::setBitOrder( uint8_t bit_order )
{
	// Unsupported by rpi
}


void nRF24::SPI::setDataMode( uint8_t data_mode )
{
}


void nRF24::SPI::setClockDivider( uint16_t spi_speed )
{
}


void nRF24::SPI::chipSelect( int csn_pin )
{
}


uint8_t nRF24::SPI::transfer( uint8_t _data )
{
	uint8_t data = 0;
	transfernb( (char*)&_data, (char*)&data, 1 );
	return data;
}


void nRF24::SPI::transfernb( char* tbuf, char* rbuf, uint32_t len )
{
	mXFer[0].tx_buf = (uintptr_t)tbuf;
	mXFer[0].len = len;
	mXFer[0].rx_buf = (uintptr_t)rbuf;
	int ret = ioctl( mFD, SPI_IOC_MESSAGE(1), mXFer );
	if ( ret <= 0 ) {
		printf( "transfernb failed : %d (%s)\n", errno, strerror(errno) );
	} else {
// 		printf( "transfernb(%p, %p, %d) ok : %d\n", tbuf, rbuf, len, ret );
	}
}


void nRF24::SPI::transfern( char* buf, uint32_t len )
{
	char* rbuf = new char[len];
	transfernb( buf, rbuf, len );
	memcpy( buf, rbuf, len );
	delete [] rbuf;
}