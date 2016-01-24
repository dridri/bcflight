#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <pthread.h>
#include <list>

class I2C
{
public:
	I2C( int addr );
	~I2C();

	int Read( uint8_t reg, void* buf, uint32_t len );
	int Write( uint8_t reg, void* buf, uint32_t len );
	int Read8( uint8_t reg, uint8_t* value );
	int Read16( uint8_t reg, uint16_t* value );
	int Read32( uint8_t reg, uint32_t* value );
	int Write8( uint8_t reg, uint8_t value );
	int Write16( uint8_t reg, uint16_t value );
	int Write32( uint8_t reg, uint32_t value );

	static std::list< int > ScanAll();

private:
	int mAddr;
	static int mFD;
	static int mCurrAddr;
	static pthread_mutex_t mMutex;
	int _Read( int addr, uint8_t reg, void* buf, uint32_t len );
	int _Write( int addr, uint8_t reg, void* buf, uint32_t len );
};

#endif
