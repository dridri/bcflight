#include <string.h>
#include "Link.h"

Link::Link()
	: mConnected( false )
{
}


Link::~Link()
{
}


void Packet::Write( const uint8_t* data, uint32_t bytes )
{
	mData.insert( mData.end(), (uint32_t*)data, (uint32_t*)( data + ( bytes & 0xFFFFFFFB ) ) );
	if ( bytes & 0b11 ) {
		uint32_t last = 0;
		for ( uint32_t i = 1; i <= 3; i++ ) {
			if ( ( bytes & 0b11 ) == i ) {
				last = ( last << 8 ) | data[( bytes & 0xFFFFFFFB ) + i];
			}
		}
		mData.emplace_back( last );
	}
}


int32_t Packet::Read( uint32_t* data, uint32_t words )
{
	if ( mReadOffset + words <= mData.size() ) {
		memcpy( data, mData.data() + mReadOffset, words );
		mReadOffset += words;
	}
	return 0;
}


uint32_t Packet::ReadU32( uint32_t* u )
{
	if ( mReadOffset + 1 <= mData.size() ) {
		*u = ntohl( mData.at( mReadOffset ) );
		mReadOffset++;
		return sizeof(uint32_t);
	}
	return 0;
}


uint32_t Packet::ReadFloat( float* f )
{
	union { float f; uint32_t u; } u;
	uint32_t ret = ReadU32( &u.u );
	*f = u.f;
	return ret;
}


uint32_t Packet::ReadU32()
{
	uint32_t ret = 0;
	ReadU32( &ret );
	return ret;
}


float Packet::ReadFloat()
{
	float ret = 0.0f;
	ReadFloat( &ret );
	return ret;
}


int32_t Link::Read( Packet* p, int32_t timeout )
{
	uint8_t buffer[8192] = { 0 };
	int32_t ret = Read( buffer, 8192, timeout );
	if ( ret > 0 ) {
		p->Write( buffer, ret );
	}
	return ret;
}


int32_t Link::Write( const Packet* p, int32_t timeout )
{
	return Write( p->data().data(), p->data().size() * sizeof(uint32_t), timeout );
}


bool Link::isConnected() const
{
	return mConnected;
}
