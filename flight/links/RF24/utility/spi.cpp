#include "spi.h"
#include <pthread.h>
#include <string.h>
#include <SPI.h>

static ::SPI* mSPI = nullptr;
static std::string mSPIDevice = "";
static pthread_mutex_t spiMutex = PTHREAD_MUTEX_INITIALIZER;

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
	if ( mSPI == nullptr ) {
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
		mSPI = new ::SPI( mSPIDevice, speed );
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
	mSPI->Transfer( &_data, &data, 1 );
	return data;
}


void nRF24::SPI::transfernb(char* tbuf, char* rbuf, uint32_t len)
{
	mSPI->Transfer( tbuf, rbuf, len );
}


void nRF24::SPI::transfern(char* buf, uint32_t len)
{
	char* rbuf = new char[len];
	mSPI->Transfer( buf, rbuf, len );
	memcpy( buf, rbuf, len );
	delete rbuf;
}