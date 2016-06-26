/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <Debug.h>
#include "I2C.h"
#include "Board.h"

int I2C::mFD = -1;
int I2C::mCurrAddr = -1;
pthread_mutex_t I2C::mMutex = PTHREAD_MUTEX_INITIALIZER;

I2C::I2C( int addr )
	: mAddr( addr )
{
	pthread_mutex_lock( &mMutex );
	if ( mFD < 0 ) {
		mFD = open( "/dev/i2c-1", O_RDWR );
		gDebug() << "fd : " << mFD << "\n";
	}
	pthread_mutex_unlock( &mMutex );
}


I2C::~I2C()
{
}


const int I2C::address() const
{
	return mAddr;
}


int I2C::_Read( int addr, uint8_t reg, void* buf, uint32_t len )
{
// 	uint64_t tick = Board::GetTicks();

	pthread_mutex_lock( &mMutex );

// 	printf( "t1 : %llu\n", Board::GetTicks() - tick ); tick = Board::GetTicks();

	if ( mCurrAddr != addr ) {
		mCurrAddr = addr;
		ioctl( mFD, I2C_SLAVE, addr );
	}
// 	printf( "t2 : %llu\n", Board::GetTicks() - tick ); tick = Board::GetTicks();

	write( mFD, &reg, 1 );
// 	printf( "t3 : %llu\n", Board::GetTicks() - tick ); tick = Board::GetTicks();
	int ret = read( mFD, buf, len );
// 	printf( "t4 : %llu\n", Board::GetTicks() - tick ); tick = Board::GetTicks();

	pthread_mutex_unlock( &mMutex );
// 	printf( "t5 : %llu\n", Board::GetTicks() - tick ); tick = Board::GetTicks();
	return ret;
}


int I2C::_Write( int addr, uint8_t reg, void* _buf, uint32_t len )
{
	int ret;
// 	char* buf = new char[ len + 1 ];
	char buf[16];
	buf[0] = reg;
	memcpy( buf + 1, _buf, len );

	pthread_mutex_lock( &mMutex );

	if ( mCurrAddr != addr ) {
		mCurrAddr = addr;
		ioctl( mFD, I2C_SLAVE, addr );
	}

	ret = write( mFD, buf, len + 1 );

	pthread_mutex_unlock( &mMutex );
// 	delete buf;
	return ret;
}



int I2C::Read( uint8_t reg, void* buf, uint32_t len )
{
	return _Read( mAddr, reg, buf, len );
}


int I2C::Write( uint8_t reg, void* buf, uint32_t len )
{
	return _Write( mAddr, reg, buf, len );
}


int I2C::Read8( uint8_t reg, uint8_t* value )
{
	return Read( reg, value, 1 );
}


int I2C::Read16( uint8_t reg, uint16_t* value, bool big_endian )
{
	int ret = Read( reg, value, 2 );
	if ( big_endian ) {
		*value = __bswap_16( *value );
	}
	return ret;
}


int I2C::Read16( uint8_t reg, int16_t* value, bool big_endian )
{
	union {
		uint16_t u;
		int16_t s;
	} v;
	int ret = Read16( reg, &v.u, big_endian );
	*value = v.s;
	return ret;
}


int I2C::Read32( uint8_t reg, uint32_t* value )
{
	return Read( reg, value, 4 );
}


int I2C::Write8( uint8_t reg, uint8_t value )
{
	return Write( reg, &value, 1 );
}


int I2C::Write16( uint8_t reg, uint16_t value )
{
	return Write( reg, &value, 2 );
}


int I2C::Write32( uint8_t reg, uint32_t value )
{
	return Write( reg, &value, 4 );
}


std::list< int > I2C::ScanAll()
{
	std::list< int > ret;
	int fd = open( "/dev/i2c-1", O_RDWR );
	int res = 0;
	int byte_ = 0;

	for ( int i = 0; i < 128; i++ ) {
		if ( ioctl( fd, I2C_SLAVE, i ) < 0) {
			if ( errno == EBUSY ) {
				continue;
			} else {
				gDebug() << "Error: Could not set address to " << i << " : " << strerror(errno);
				exit(0);
			}
		}
		res = read( fd, &byte_, 1 );
		/*
		if ( ( i >= 0x30 && i <= 0x37 ) || ( i >= 0x50 && i <= 0x5F ) ) {
			res = read( fd, &byte_, 1 );
		} else {
			byte_ = 0x0F;
			res = write( fd, &byte_, 1 );
		}
		*/
		if ( res > 0 ) {
			ret.push_back( i );
		}
	}

	close( fd );
	return ret;
}
