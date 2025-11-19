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

#include "Link.h"
#include <Debug.h>
#include <Config.h>

map< string, function< Link* ( Config*, const string& ) > > Link::mKnownLinks;
list< Link* > Link::sLinks;

Link::Link()
	: mConnected( false )
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
	mData.insert( mData.end(), &data[0], &data[bytes] );
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
	u.u = board_htons( v );
	mData.insert( mData.end(), u.b, u.b + sizeof(uint16_t) );
}


void Packet::WriteU32( uint32_t v )
{
	union {
		uint32_t u;
		uint8_t b[4];
	} u;
	u.u = board_htonl( v );
	mData.insert( mData.end(), u.b, u.b + sizeof(uint32_t) );
}


void Packet::WriteString( const string& str )
{
	Write( (const uint8_t*)str.c_str(), str.length() );
}


uint32_t Packet::Read( uint8_t* data, uint32_t bytes )
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
		*u = board_ntohs( *u );
		mReadOffset += sizeof(uint16_t);
		return sizeof(uint16_t);
	}
	*u = 0;
	return 0;
}


uint32_t Packet::ReadU32( uint32_t* u )
{
	if ( mReadOffset + sizeof(uint32_t) <= mData.size() ) {
		*u = ((uint32_t*)(mData.data() + mReadOffset))[0];
		*u = board_ntohl( *u );
		mReadOffset += sizeof(uint32_t);
		return sizeof(uint32_t);
	}
	*u = 0;
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


string Packet::ReadString()
{
	string res = "";
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


SyncReturn Link::Read( Packet* p, int32_t timeout )
{
	uint8_t buffer[8192] = { 0 };
	int32_t ret = Read( buffer, 8192, timeout );
	if ( ret > 0 ) {
		p->Write( buffer, ret );
	}
	return ret;
}


SyncReturn Link::Write( const Packet* p, bool ack, int32_t timeout )
{
	return Write( p->data().data(), p->data().size(), ack, timeout );
}


bool Link::isConnected() const
{
	return mConnected;
}


Link* Link::Create( Config* config, const string& lua_object )
{
	Link* instance = (Link*)strtoull( config->String( lua_object + "._instance" ).c_str(), nullptr, 10 );
	if ( instance ) {
		gDebug() << "Reselecting already existing Link";
		return instance;
	}

	string type = config->String( lua_object + ".link_type", "" );
	Link* ret = nullptr;


	if ( type != "" and mKnownLinks.find( type ) != mKnownLinks.end() ) {
		ret = mKnownLinks[ type ]( config, lua_object );
	}

	if ( ret != nullptr ) {
		config->Execute( lua_object + "._instance = \"" + to_string( (uintptr_t)ret ) + "\"" );
		return ret;
	}

	gDebug() << "Error : Link type \"" << type << "\" not supported !";
	return nullptr;
} // -- Socket{ type = "TCP/UDP/UDPLite", port = port_number[, broadcast = true/false] } <= broadcast is false by default


void Link::RegisterLink( const string& name, function< Link* ( Config*, const string& ) > instanciate )
{
	fDebug( name );
	mKnownLinks[ name ] = instanciate;
}


LuaValue Link::infosAll()
{
	LuaValue ret;

	for ( auto link : sLinks ) {
		ret[ link->name() ] = link->infos();
	}

	return ret;
}
