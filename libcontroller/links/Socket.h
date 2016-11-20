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

#ifndef SOCKET_H
#define SOCKET_H

#ifdef WIN32
#include <winsock2.h>
#define socklen_t int
#else
#include <netinet/in.h>
#endif
#include "Link.h"

class Main;

class Socket : public Link
{
public:
	typedef enum {
		TCP,
		UDP,
		UDPLite
	} PortType;

	Socket( const std::string& host, uint16_t port, PortType type = TCP );
	virtual ~Socket();

	int Connect();
	int setBlocking( bool blocking );

protected:
	int Read( void* buf, uint32_t len, int32_t timeout );
	int Write( const void* buf, uint32_t len, int32_t timeout );

	std::string mHost;
	uint16_t mPort;
	PortType mPortType;
	int mSocket;
	struct sockaddr_in mSin;
};

#endif // ( BUILD_SOCKET == 1 )
