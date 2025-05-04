#include "Bus.h"
#include <Debug.h>

Bus::Bus()
	: mConnected( false )
{
}


Bus::~Bus()
{
}


int Bus::Read8( uint8_t reg, uint8_t* value )
{
	return Read( reg, value, 1 );
}


int Bus::Read16( uint8_t reg, uint16_t* value, bool big_endian )
{
	int ret = Read( reg, value, 2 );
	if ( big_endian ) {
#ifdef bswap_16
		*value = bswap_16( *value );
#elif defined( __bswap16 )
		*value = __bswap16( *value );
#else
		*value = ( (*value) >> 8 ) | ( (*value) << 8 );
#endif
	}
	return ret;
}


int Bus::Read16( uint8_t reg, int16_t* value, bool big_endian )
{
	union {
		uint16_t u;
		int16_t s;
	} v;
	int ret = Read16( reg, &v.u, big_endian );
	*value = v.s;
	return ret;
}


int Bus::Read24( uint8_t reg, uint32_t* value )
{
	uint8_t v[3] = { 0, 0, 0 };

	int ret = Read( reg, v, 3 );

	*value = ( ((uint32_t)v[0]) << 16 ) | ( ((uint32_t)v[1]) << 8 ) | ( ((uint32_t)v[2]) << 0 );
	return ret;
}


int Bus::Read32( uint8_t reg, uint32_t* value )
{
	return Read( reg, value, 4 );
}


int Bus::Write8( uint8_t reg, uint8_t value )
{
	return Write( reg, &value, 1 );
}


int Bus::Write16( uint8_t reg, uint16_t value )
{
	return Write( reg, &value, 2 );
}


int Bus::Write32( uint8_t reg, uint32_t value )
{
	return Write( reg, &value, 4 );
}


int Bus::WriteFloat( uint8_t reg, float value )
{
	union {
		uint32_t u;
		float f;
	} v;
	v.f = value;
	return Write( reg, &v.u, 4 );
}


int Bus::WriteVector3f( uint8_t reg, const Vector3f& vec )
{
	typedef union {
		uint32_t u;
		float f;
	} v;
	struct {
		v x, y, z;
	} __attribute__((packed)) uvec;
	uvec.x.f = vec.x;
	uvec.y.f = vec.y;
	uvec.z.f = vec.z;
	return Write( reg, &uvec.x.u, 4 * 3 );
}
