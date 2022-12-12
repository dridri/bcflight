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

I2C::I2C( int addr, bool slave )
	: Bus()
	, mAddr( addr )
{
}


I2C::~I2C()
{
}


int I2C::Connect()
{
	pthread_mutex_lock( &mMutex );
	if ( mFD < 0 ) {
		mFD = open( "/dev/i2c-1", O_RDWR );
		gDebug() << "fd : " << mFD;
	}
	pthread_mutex_unlock( &mMutex );
	return 0;
}


std::string I2C::toString()
{
	stringstream ss;
	ss <<"I2C@0x" << std::hex << mAddr;
	return ss.str();
}


const int I2C::address() const
{
	return mAddr;
}


int I2C::Read( void* buf, uint32_t len )
{
	pthread_mutex_lock( &mMutex );

	if ( mCurrAddr != mAddr ) {
		mCurrAddr = mAddr;
		if ( ioctl( mFD, I2C_SLAVE, mAddr ) != 0 ) {
			return 0;
		}
	}

	int ret = read( mFD, buf, len );

	pthread_mutex_unlock( &mMutex );
	return ret;
}


int I2C::Write( const void* buf, uint32_t len )
{
	int ret;

	pthread_mutex_lock( &mMutex );

	if ( mCurrAddr != mAddr ) {
		mCurrAddr = mAddr;
		if ( ioctl( mFD, I2C_SLAVE, mAddr ) != 0 ) {
			return 0;
		}
	}

	ret = write( mFD, buf, len );

	pthread_mutex_unlock( &mMutex );
	return ret;
}


int I2C::Read( uint8_t reg, void* buf, uint32_t len )
{
	pthread_mutex_lock( &mMutex );

	if ( mCurrAddr != mAddr ) {
		mCurrAddr = mAddr;
		if ( ioctl( mFD, I2C_SLAVE, mAddr ) != 0 ) {
			return 0;
		}
	}

	write( mFD, &reg, 1 );
	int ret = read( mFD, buf, len );

	pthread_mutex_unlock( &mMutex );
	return ret;
}


int I2C::Write( uint8_t reg, const void* _buf, uint32_t len )
{
	int ret;
	char buf[1024];
	buf[0] = reg;
	memcpy( buf + 1, _buf, len );

	pthread_mutex_lock( &mMutex );

	if ( mCurrAddr != mAddr ) {
		mCurrAddr = mAddr;
		if ( ioctl( mFD, I2C_SLAVE, mAddr ) != 0 ) {
			return 0;
		}
	}

	ret = write( mFD, buf, len + 1 );

	pthread_mutex_unlock( &mMutex );
	return ret;
}


list< int > I2C::ScanAll()
{
	list< int > ret;
	int fd = open( "/dev/i2c-1", O_RDWR );
	if ( fd < 0 ) {
		gError() << "Cannot open " << string("/dev/i2c-1") << " : " << strerror(errno);
		return ret;
		
	}
	int res = 0;
	int byte_ = 0;

	for ( int i = 0; i < 128; i++ ) {
		if ( ioctl( fd, I2C_SLAVE, i ) < 0) {
			if ( errno == EBUSY ) {
				continue;
			} else {
				gError() << "Could not set address to " << i << " : " << strerror(errno);
				return ret;
			}
		}
		char buf[] = {1};
		write( fd, buf, 1 );
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
	if ( ret.size() >= 127 ) {
		gError() << "Defective I2C bus !";
		Board::defectivePeripherals()["I2C"] = true;
		ret.clear();
	}
	return ret;
}
