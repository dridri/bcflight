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

#ifdef WIN32
#include <windows.h>
#define socklen_t int
#else
#include <netinet/in.h>
#endif
#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <list>
#include "Lua.h"

class Config;

typedef int32_t SyncReturn;
#define LINK_ERROR_TIMEOUT -3
#define TIMEOUT -3
#define CONTINUE 0


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
	void WriteString( const std::string& str );

	int32_t Read( uint8_t* data, uint32_t bytes );
	uint32_t ReadU8( uint8_t* u );
	uint32_t ReadU16( uint16_t* u );
	uint32_t ReadU32( uint32_t* u );
	uint32_t ReadFloat( float* f );
	uint8_t ReadU8();
	uint16_t ReadU16();
	uint32_t ReadU32();
	float ReadFloat();
	std::string ReadString();

	const std::vector< uint8_t >& data() const { return mData; }

private:
	std::vector< uint8_t > mData;
	uint32_t mReadOffset;
};


class Link
{
public:
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
	int32_t Read( Packet* p, int32_t timeout = 0 );
	int32_t Write( const Packet* p, bool ack = false, int32_t timeout = 0 );

	virtual int Read( void* buf, uint32_t len, int32_t timeout ) = 0;
	virtual int Write( const void* buf, uint32_t len, bool ack, int32_t timeout ) = 0;

	uint32_t writeSpeed() const { return mWriteSpeed; }
	uint32_t readSpeed() const { return mReadSpeed; }
	virtual uint32_t fullReadSpeed() = 0;

	virtual string name() const { return "Link"; }
	virtual LuaValue infos() const { return LuaValue(); }

	static std::list< Link* > links() { return sLinks; }
	static LuaValue infosAll();

protected:
	bool mConnected;
	uint32_t mReadSpeed;
	uint32_t mWriteSpeed;
	uint32_t mReadSpeedCounter;
	uint32_t mWriteSpeedCounter;
	uint64_t mSpeedTick;

	uint64_t GetTicks();

	static std::list< Link* > sLinks;
};

#endif // LINK_H
