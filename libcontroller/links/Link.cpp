#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "Link.h"

Link::Link()
	: mConnected( false )
	, mReadSpeed( 0 )
	, mWriteSpeed( 0 )
	, mReadSpeedCounter( 0 )
	, mWriteSpeedCounter( 0 )
	, mSpeedTick( 0 )
{
}


Link::~Link()
{
}


void Packet::Write( const uint8_t* data, uint32_t bytes )
{
	mData.insert( mData.end(), (uint32_t*)data, (uint32_t*)( data + ( bytes & 0xFFFFFFFC ) ) );
	if ( bytes & 0b11 ) {
		uint32_t last = 0;
		for ( uint32_t i = 0; i <= 3 and ( bytes & 0xFFFFFFFC ) + i < bytes; i++ ) {
			last = ( last << 8 ) | data[( bytes & 0xFFFFFFFC ) + i];
		}
		last = htonl( last );
		mData.emplace_back( last );
	}
}


void Packet::WriteString( const std::string& str )
{
	Write( (const uint8_t*)str.c_str(), str.length() );
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


std::string Packet::ReadString()
{
	std::string res = "";
	uint32_t i = 0;

	for ( i = 0; mReadOffset + i < mData.size(); i++ ) {
		uint32_t word = mData.at( mReadOffset + i );
		char a = (char)( word & 0x000000FF );
		char b = (char)( ( word & 0x0000FF00 ) >> 8 );
		char c = (char)( ( word & 0x00FF0000 ) >> 16 );
		char d = (char)( ( word & 0xFF000000 ) >> 24 );
		if ( a ) res += a;
		if ( b ) res += b;
		if ( c ) res += c;
		if ( d ) res += d;
	}

	mReadOffset += i;
	return res;
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


uint64_t Link::GetTicks()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}
