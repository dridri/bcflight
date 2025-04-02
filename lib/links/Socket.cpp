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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef WIN32
#include <winsock2.h>
#define socklen_t int
#define DATATYPE char*
#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif
#else
#define DATATYPE void*
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <iwlib.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#endif
#include <iostream>
#include "Socket.h"

#ifndef IPPROTO_UDPLITE
#define IPPROTO_UDPLITE 136
#endif
#define UDPLITE_SEND_CSCOV   10 /* sender partial coverage (as sent)      */
#define UDPLITE_RECV_CSCOV   11 /* receiver partial coverage (threshold ) */


Socket::Socket( const std::string& host, uint16_t port, PortType type )
	: mHost( host )
	, mPort( port )
	, mPortType( type )
	, mSocket( -1 )
	, mChannel( 0 )
{
	iwstats stats;
	wireless_config info;
	iwrange range;
	int iwSocket = iw_sockets_open();

	memset( &stats, 0, sizeof( stats ) );
	if ( iw_get_basic_config( iwSocket, "wlan0", &info ) == 0 ) {
		if ( iw_get_range_info( iwSocket, "wlan0", &range ) == 0 ) {
			mChannel = iw_freq_to_channel( info.freq, &range );
		}
	}

	iw_sockets_close( iwSocket );
}


Socket::~Socket()
{
	if ( mConnected and mSocket >= 0 ) {
		shutdown( mSocket, 2 );
		closesocket( mSocket );
	}
}


int32_t Socket::Channel()
{
	return mChannel;
}


int32_t Socket::RxQuality()
{
	iwstats stats;

	int32_t ret = 0;
	int iwSocket = iw_sockets_open();
	memset( &stats, 0, sizeof( stats ) );

	if ( iw_get_stats( iwSocket, "wlan0", &stats, nullptr, 0 ) == 0 ) {
		ret = (int32_t)stats.qual.qual * 100 / 70;
	}

	iw_sockets_close( iwSocket );
	return ret;
}


int32_t Socket::RxLevel()
{
	iwstats stats;

	int32_t ret = 0;
	int iwSocket = iw_sockets_open();
	memset( &stats, 0, sizeof( stats ) );

	if ( iw_get_stats( iwSocket, "wlan0", &stats, nullptr, 0 ) == 0 ) {
		union { int8_t s; uint8_t u; } conv = { .u = stats.qual.level };
		ret = conv.s;
	}

	iw_sockets_close( iwSocket );
	return ret;
}


int Socket::Connect()
{
	if ( mConnected ) {
		return 0;
	}
	setBlocking( true );

	if ( mSocket < 0 ) {
		int type = ( mPortType == UDP or mPortType == UDPLite ) ? SOCK_DGRAM : SOCK_STREAM;
		int proto = ( mPortType == UDPLite ) ? IPPROTO_UDPLITE : ( ( mPortType == UDP ) ? IPPROTO_UDP : 0 );

		char myname[256];
		gethostname( myname, sizeof(myname) );
		memset( &mSin, 0, sizeof( mSin ) );
		mSin.sin_addr.s_addr = inet_addr( mHost.c_str() );
		mSin.sin_family = AF_INET;
		mSin.sin_port = htons( mPort );

		mSocket = socket( AF_INET, type, proto );
		int option = 1;
		setsockopt( mSocket, SOL_SOCKET, ( 15/*SO_REUSEPORT*/ | SO_REUSEADDR ), (char*)&option, sizeof( option ) );
		if ( mPortType == TCP ) {
			int flag = 1;
			setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int) );
		}
		if ( mPortType == UDPLite ) {
			uint16_t checksum_coverage = 8;
			setsockopt( mSocket, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV, (DATATYPE)&checksum_coverage, sizeof(checksum_coverage) );
			setsockopt( mSocket, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV, (DATATYPE)&checksum_coverage, sizeof(checksum_coverage) );
		}
		if ( connect( mSocket, (SOCKADDR*)&mSin, sizeof(mSin) ) < 0 ) {
			std::cout << "Socket ( " << mPort << " ) connect error : " << strerror(errno) << "\n";
			close( mSocket );
			mSocket = -1;
			mConnected = false;
			return -1;
		}
	}

	mConnected = true;
	return 0;
}


int Socket::setBlocking( bool blocking )
{
#ifdef WIN32
	unsigned long nonblock = !blocking;
	return ( ioctlsocket( mSocket, FIONBIO, &nonblock ) == 0 );
#else
	int flags = fcntl( mSocket, F_GETFL, 0 );
	flags = blocking ? ( flags & ~O_NONBLOCK) : ( flags | O_NONBLOCK );
	return ( fcntl( mSocket, F_SETFL, flags ) == 0 );
#endif
}


int Socket::retriesCount() const
{
	// TODO
	return 1;
}


void Socket::setRetriesCount( int retries )
{
	if ( mPortType == UDP or mPortType == UDPLite ) {
		// TODO : set retries count
	}
}


int Socket::Read( void* buf, uint32_t len, int timeout )
{
	if ( !mConnected ) {
		return -1;
	}

	int ret = 0;
	memset( buf, 0, len );

// 	timeout = 500;
	if ( timeout > 0 ) {
		struct timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = 1000 * ( timeout % 1000 );
		setsockopt( mSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval) );
	}

	if ( mPortType == UDP or mPortType == UDPLite ) {
		socklen_t fromsize = sizeof( mSin );
		ret = recvfrom( mSocket, (DATATYPE)buf, len, 0, (SOCKADDR *)&mSin, &fromsize );
		if ( ret <= 0 and errno != EAGAIN) {
			std::cout << "UDP disconnected ( " << ret << " : " << strerror( errno ) << " )\n";
			mConnected = false;
			return -1;
		}
// 		return -1;
	} else {
		ret = recv( mSocket, (DATATYPE)buf, len, MSG_NOSIGNAL );
// 		if ( ( ret <= 0 and errno != EAGAIN ) or ( errno == EAGAIN and timeout > 0 ) ) {
		if ( ret <= 0 ) {
			std::cout << "TCP disconnected ( " << strerror( errno ) << " )\n";
			mConnected = false;
			mSocket = -1;
			return -1;
		}
	}

	if ( ret > 0 ) {
		mReadSpeedCounter += ret;
	}
	if ( GetTicks() - mSpeedTick >= 1000 * 1000 ) {
		mReadSpeed = mReadSpeedCounter;
		mWriteSpeed = mWriteSpeedCounter;
		mReadSpeedCounter = 0;
		mWriteSpeedCounter = 0;
		mSpeedTick = GetTicks();
	}
	return ret;
}


int Socket::Write( const void* buf, uint32_t len, bool ack, int timeout )
{
	if ( !mConnected ) {
		return -1;
	}

	int ret = 0;

	if ( mPortType == UDP or mPortType == UDPLite ) {
		uint32_t sendsize = sizeof( mSin );
		ret = sendto( mSocket, (DATATYPE)buf, len, 0, (SOCKADDR *)&mSin, sendsize );
	} else {
		ret = send( mSocket, (DATATYPE)buf, len, 0 );
	}

	if ( ret <= 0 and ( errno == EAGAIN or errno == -EAGAIN ) ) {
		return 0;
	}

	if ( ret < 0 and mPortType != UDP and mPortType != UDPLite ) {
		std::cout << "TCP disconnected\n";
		mConnected = false;
		mSocket = -1;
		return -1;
	}

	if ( ret > 0 ) {
		mWriteSpeedCounter += ret;
	}
	if ( GetTicks() - mSpeedTick >= 1000 * 1000 ) {
		mReadSpeed = mReadSpeedCounter;
		mWriteSpeed = mWriteSpeedCounter;
		mReadSpeedCounter = 0;
		mWriteSpeedCounter = 0;
		mSpeedTick = GetTicks();
	}
	return ret;
}
