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

#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "Link.h"

list< Link* > Link::sLinks;

Link::Link()
	: mConnected( false )
	, mReadSpeed( 0 )
	, mWriteSpeed( 0 )
	, mReadSpeedCounter( 0 )
	, mWriteSpeedCounter( 0 )
	, mSpeedTick( 0 )
{
	sLinks.emplace_back( this );
}


Link::~Link()
{
}


void Packet::Write( const uint8_t* data, uint32_t bytes )
{
/*
	mData.insert( mData.end(), (uint32_t*)data, (uint32_t*)( data + ( bytes & 0xFFFFFFFC ) ) );
	if ( bytes & 0b11 ) {
		uint32_t last = 0;
		for ( uint32_t i = 0; i <= 3 and ( bytes & 0xFFFFFFFC ) + i < bytes; i++ ) {
			last = ( last << 8 ) | data[( bytes & 0xFFFFFFFC ) + i];
		}
		last = htonl( last );
		mData.emplace_back( last );
		mData.emplace_back( 0 );
	}
*/
	mData.insert( mData.end(), data, data + bytes );
}


void Packet::WriteU8( uint8_t v )
{
	mData.insert( mData.end(), &v, &v + sizeof(uint8_t) );
}


void Packet::WriteU16( uint16_t v )
{
	union {
		uint16_t u;
		uint8_t b[2];
	} u;
	u.u = htons( v );
	mData.insert( mData.end(), u.b, u.b + sizeof(uint16_t) );
}


void Packet::WriteU32( uint32_t v )
{
	union {
		uint32_t u;
		uint8_t b[4];
	} u;
	u.u = htonl( v );
	mData.insert( mData.end(), u.b, u.b + sizeof(uint32_t) );
}


void Packet::WriteString( const std::string& str )
{
	Write( (const uint8_t*)str.c_str(), str.length()+1 );
}


int32_t Packet::Read( uint8_t* data, uint32_t bytes )
{
	if ( mReadOffset + bytes <= mData.size() ) {
		memcpy( data, mData.data() + mReadOffset, bytes );
		mReadOffset += bytes;
		return bytes;
	}
	memset( data, 0, bytes );
	return 0;
}


uint32_t Packet::ReadU8( uint8_t* u )
{
	if ( mReadOffset + sizeof(uint8_t) <= mData.size() ) {
		*u = ((uint8_t*)(mData.data() + mReadOffset))[0];
		mReadOffset += sizeof(uint8_t);
		return sizeof(uint8_t);
	}
	*u = 0;
	return 0;
}


uint32_t Packet::ReadU16( uint16_t* u )
{
	if ( mReadOffset + sizeof(uint16_t) <= mData.size() ) {
		*u = ((uint16_t*)(mData.data() + mReadOffset))[0];
		*u = ntohs( *u );
		mReadOffset += sizeof(uint16_t);
		return sizeof(uint16_t);
	}
	*u = 0;
	return 0;
}


uint32_t Packet::ReadU32( uint32_t* u )
{
	if ( mReadOffset + sizeof(uint32_t) <= mData.size() ) {
		*u = ntohl( ((uint32_t*)(mData.data() + mReadOffset))[0] );
		mReadOffset += sizeof(uint32_t);
		return sizeof(uint32_t);
	}
	*u = 0;
	return 0;
}


uint32_t Packet::ReadFloat( float* f )
{
	union { float f; uint32_t u; } u;
	u.u = 0;
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


uint16_t Packet::ReadU16()
{
	uint16_t ret = 0;
	ReadU16( &ret );
	return ret;
}


uint8_t Packet::ReadU8()
{
	uint8_t ret = 0;
	ReadU8( &ret );
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
		uint8_t c = mData.at( mReadOffset + i );
		if ( c ) {
			res += (char)c;
		} else {
			break;
		}
	}

	mReadOffset += i;
	return res;
}


int32_t Link::Read( Packet* p, int32_t timeout )
{
	uint8_t buffer[65536] = { 0 };
	int32_t ret = Read( buffer, 65536, timeout );
	if ( ret > 0 ) {
		p->Write( buffer, ret );
	}
	return ret;
}


int32_t Link::Write( const Packet* p, bool ack, int32_t timeout )
{
	return Write( p->data().data(), p->data().size(), ack, timeout );
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


LuaValue Link::infosAll()
{
	LuaValue ret;

	for ( auto link : sLinks ) {
		ret[ link->name() ] = link->infos();
	}

	return ret;
}
