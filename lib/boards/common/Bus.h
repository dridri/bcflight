#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "Vector.h"
#include "Lua.h"
#include <string>
#include <list>
#include <vector>

class Bus
{
public:
	Bus();
	virtual ~Bus();

	virtual int Connect() = 0;

	virtual int Read( void* buf, uint32_t len ) = 0;
	virtual int Write( const void* buf, uint32_t len ) = 0;
	virtual int Read( uint8_t reg, void* buf, uint32_t len ) = 0;
	virtual int Write( uint8_t reg, const void* buf, uint32_t len ) = 0;

	int Read8( uint8_t reg, uint8_t* value );
	int Read16( uint8_t reg, uint16_t* value, bool big_endian = false );
	int Read16( uint8_t reg, int16_t* value, bool big_endian = false );
	int Read24( uint8_t reg, uint32_t* value );
	int Read32( uint8_t reg, uint32_t* value );
	int Write8( uint8_t reg, uint8_t value );
	int Write16( uint8_t reg, uint16_t value );
	int Write32( uint8_t reg, uint32_t value );
	int WriteFloat( uint8_t reg, float value );
	int WriteVector3f( uint8_t reg, const Vector3f& vec );

	virtual std::string toString() { return std::string(); }
	virtual LuaValue infos() { return LuaValue(); }

protected:
	bool mConnected;
};

#endif // BUS_H
