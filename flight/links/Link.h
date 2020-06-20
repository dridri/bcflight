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

#ifndef LINK_H
#define LINK_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <Board.h>
#include <ThreadBase.h>

class Config;


class Packet
{
public:
	Packet() : mReadOffset( 0 ) {}
	Packet( uint32_t id ) { WriteU16( id ); }
	void Write( const uint8_t* data, uint32_t bytes );
	void WriteU8( uint8_t v );
	void WriteU16( uint16_t v );
	void WriteU32( uint32_t v );
	void WriteFloat( float v ) { union { float f; uint32_t u; } u; u.f = v; WriteU32( u.u ); }
	void WriteString( const string& str );

	uint32_t Read( uint8_t* data, uint32_t bytes );
	uint32_t ReadU8( uint8_t* u );
	uint32_t ReadU16( uint16_t* u );
	uint32_t ReadU32( uint32_t* u );
	uint32_t ReadFloat( float* f );
	uint8_t ReadU8();
	uint16_t ReadU16();
	uint32_t ReadU32();
	float ReadFloat();
	string ReadString();

	const vector< uint8_t >& data() const { return mData; }

private:
	vector< uint8_t > mData;
	uint32_t mReadOffset;
};


class Link
{
public:
	static Link* Create( Config* config, const string& config_object );
	Link();
	virtual ~Link();

	bool isConnected() const;

	virtual int Connect() = 0;
	virtual int setBlocking( bool blocking ) = 0;
	virtual void setRetriesCount( int retries ) = 0;
	virtual int retriesCount() const = 0;
	virtual int32_t Channel() { return 0; }
	virtual int32_t Frequency() { return 0; }
	virtual int32_t RxQuality() { return 100; }
	virtual int32_t RxLevel() { return -1; }

	SyncReturn Read( Packet* p, int32_t timeout = -1 );
	SyncReturn Write( const Packet* p, bool ack = false, int32_t timeout = -1 );
	virtual SyncReturn Read( void* buf, uint32_t len, int32_t timeout ) = 0;
	virtual SyncReturn Write( const void* buf, uint32_t len, bool ack = false, int32_t timeout = -1 ) = 0;

	virtual SyncReturn WriteAck( const void* buf, uint32_t len ) { return Write( buf, len, false, -1 ); }

	static const map< string, function< Link* ( Config*, const string& ) > >& knownLinks() { return mKnownLinks; }

protected:
	bool mConnected;


	static void RegisterLink( const string& name, function< Link* ( Config*, const string& ) > instanciate );
	static map< string, function< Link* ( Config*, const string& ) > > mKnownLinks;
};

#endif // LINK_H
