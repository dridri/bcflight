#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "MCP320x.h"

#include <gammaengine/Debug.h>

using namespace GE;


MCP320x::MCP320x()
{
	mFD = open( "/dev/spidev32766.0", O_RDWR );

	uint8_t mode, lsb, bits;
	uint32_t speed = 500000;

	mode = SPI_MODE_0;
	if ( ioctl( mFD, SPI_IOC_WR_MODE, &mode ) < 0 )
	{
		gDebug() << "SPI rd_mode\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_RD_MODE, &mode ) < 0 )
	{
		gDebug() << "SPI rd_mode\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_RD_LSB_FIRST, &lsb ) < 0 )
	{
		gDebug() << "SPI rd_lsb_fist\n";
		return;
	}
	if ( ioctl( mFD, SPI_IOC_RD_BITS_PER_WORD, &bits ) < 0 ) 
	{
		gDebug() << "SPI bits_per_word\n";
		return;
	}

	if ( ioctl( mFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed ) < 0 )  
	{
		gDebug() << "can't set max speed hz\n";
		return;
	}

	if ( ioctl( mFD, SPI_IOC_RD_MAX_SPEED_HZ, &speed ) < 0 ) 
	{
		gDebug() << "SPI max_speed_hz\n";
		return;
	}

    printf( "/dev/spidev32766.0: spi mode %d, %d bits %sper word, %d Hz max\n", mode, bits, lsb ? "(lsb first) " : "", speed );

	memset( mXFer, 0, sizeof( mXFer ) );

	for ( int i = 0; i < sizeof( mXFer ) /  sizeof( struct spi_ioc_transfer ); i++) {
		mXFer[i].len = 0;
		mXFer[i].cs_change = 0; // Keep CS activated
		mXFer[i].delay_usecs = 0;
		mXFer[i].speed_hz = 500000;
		mXFer[i].bits_per_word = 8;
	}
}


MCP320x::~MCP320x()
{
}



uint16_t MCP320x::Read( uint8_t channel )
{
	int status;
	static const int nbx = 4;
	uint32_t b[3] = { 0, 0, 0 };
	uint8_t bx[nbx][3] = { { 0 } };
	uint8_t buf[16];

	buf[0] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[1] = ( ( channel & 0b11 ) << 6 );
	buf[2] = 0x00;
	buf[3] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[4] = ( ( channel & 0b11 ) << 6 );
	buf[5] = 0x00;
	buf[6] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[7] = ( ( channel & 0b11 ) << 6 );
	buf[8] = 0x00;
	buf[9] = 0b00000110 | ( ( channel & 0b100 ) >> 2 );
	buf[10] = ( ( channel & 0b11 ) << 6 );
	buf[11] = 0x00;
/*
	mXFer[0].tx_buf = (uintptr_t)buf;
	mXFer[0].len = 12;
	mXFer[0].rx_buf = (uintptr_t)bx;
	status = ioctl( mFD, SPI_IOC_MESSAGE(1), mXFer );
*/
	for ( int i = 0; i < nbx; i++ ) {
		mXFer[0].tx_buf = (uintptr_t)buf;
		mXFer[0].len = 3;
		mXFer[0].rx_buf = (uintptr_t)bx[i];
		status = ioctl( mFD, SPI_IOC_MESSAGE(1), mXFer );
	}

	for ( int j = 0; j < nbx; j++ ) {
		for ( int i = 0; i < nbx; i++ ) {
			if ( i != j and bx[i][1] != bx[j][1] ) {
				return 0;
			}
		}
	}
	for ( int i = 0; i < nbx; i++ ) {
		b[0] += bx[i][0];
		b[1] += bx[i][1];
		b[2] += bx[i][2];
	}
	b[0] /= nbx;
	b[1] /= nbx;
	b[2] /= nbx;

	b[2] = b[2] & 0xF0;

	if ( channel != 7 ) {
		if ( b[1] < 0x05 ) {
			return 1500;
		}
		if ( b[1] > 0x09 ) {
			return 2500;
		}
	}
// 	else {
// 		printf( "b : { %02X %02X %02X %04X %d }\n", b[0], b[1], b[2], ( ( b[1] & 0b1111 ) << 8 ) | b[2], ( ( b[1] & 0b1111 ) << 8 ) | b[2] );
// 	}
	return ( ( b[1] & 0b1111 ) << 8 ) | b[2];
}
