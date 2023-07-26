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

#if ( BUILD_SOCKET == 1 )

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
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

#include "Debug.h"
#include "Board.h"
#include "Socket.h"
#include "../Config.h"

#define UDPLITE_SEND_CSCOV   10 /* sender partial coverage (as sent)      */
#define UDPLITE_RECV_CSCOV   11 /* receiver partial coverage (threshold ) */


Socket::Socket( uint16_t port, Socket::PortType type, bool broadcast, uint32_t timeout )
	: mPort( port )
	, mPortType( type )
	, mBroadcast( broadcast )
	, mTimeout( timeout )
	, mSocket( -1 )
	, mClientSocket( -1 )
	, mChannel( 0 )
{
	fDebug( port, type, broadcast, timeout );
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


Socket::Socket()
	: Socket( 0 )
{
	fDebug();
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
	// TODO : use "iw dev wlan0 station dump" instead
	iwstats stats;

	errno = 0;

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

	int32_t ret = -200;
	int iwSocket = iw_sockets_open();
	memset( &stats, 0, sizeof( stats ) );

	if ( iw_get_stats( iwSocket, "wlan0", &stats, nullptr, 0 ) == 0 ) {
		union { int8_t s; uint8_t u; } conv = { .u = stats.qual.level };
		ret = conv.s;
	}

	iw_sockets_close( iwSocket );
	return ret;
}


int Socket::setBlocking( bool blocking )
{
	int flags = fcntl( mSocket, F_GETFL, 0 );
	flags = blocking ? ( flags & ~O_NONBLOCK) : ( flags | O_NONBLOCK );
	return ( fcntl( mSocket, F_SETFL, flags ) == 0 );
}


int Socket::retriesCount() const
{
	if ( mPortType == UDP or mPortType == UDPLite ) {
		// TODO
	}
	return 1;
}


void Socket::setRetriesCount( int retries )
{
	if ( mPortType == UDP or mPortType == UDPLite ) {
		// TODO : set retries count
	}
}


int Socket::Connect()
{
	fDebug( mPort, mPortType );

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
		mSin.sin_addr.s_addr = htonl( INADDR_ANY );
		mSin.sin_family = AF_INET;
		mSin.sin_port = htons( mPort );

		mSocket = socket( AF_INET, type, proto );
		int option = 1;
		setsockopt( mSocket, SOL_SOCKET, ( 15/*SO_REUSEPORT*/ | SO_REUSEADDR ), (char*)&option, sizeof( option ) );
		if ( mPortType == TCP ) {
			int flag = 1;
			setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int) );
		}
		if ( mBroadcast ) {
			int broadcastEnable = 1;
			setsockopt( mSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable) );
		}
		if ( mPortType == UDPLite ) {
			uint16_t checksum_coverage = 8;
			setsockopt( mSocket, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV, &checksum_coverage, sizeof(checksum_coverage) );
			setsockopt( mSocket, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV, &checksum_coverage, sizeof(checksum_coverage) );
		}
		if ( bind( mSocket, (SOCKADDR*)&mSin, sizeof(mSin) ) < 0 ) {
			gDebug() << "Socket ( " << mPort << " ) error : " << strerror(errno);
			mConnected = false;
			return -1;
		}
	}

	if ( mPortType == TCP ) {
		int ret = listen( mSocket, 5 );
		int size = 0;
		if ( !ret ) {
			mClientSocket = accept( mSocket, (SOCKADDR*)&mClientSin, (socklen_t*)&size );
			if ( mClientSocket < 0 ) {
				mConnected = false;
				return -1;
			}
		} else {
			mConnected = false;
			return -1;
		}
	} else if ( mPortType == UDP or mPortType == UDPLite ) {
		if ( not mBroadcast ) {
			mClientSin.sin_family = AF_UNSPEC;
			/*
			uint32_t flag = 0;
			uint32_t fromsize = sizeof( mClientSin );
			int ret = recvfrom( mSocket, &flag, sizeof( flag ), 0, (SOCKADDR*)&mClientSin, &fromsize );
			if ( ret > 0 ) {
				flag = ntohl( flag );
				gDebug() << "flag : " << ntohl( flag );
				if ( flag != 0x12345678 ) {
					mConnected = false;
					return -1;
				}
			} else {
				gDebug() << strerror( errno );
				mConnected = false;
				return -1;
			}
			*/
		}
	}

	mConnected = true;
	return 0;
}


SyncReturn Socket::Read( void* buf, uint32_t len, int timeout )
{
	if ( !mConnected ) {
		return -1;
	}

	int ret = 0;
	memset( buf, 0, len );

	// If timeout is not set, default it to mTimeout
	if ( timeout <= 0 ) {
		timeout = mTimeout;
	}

	uint64_t timebase = Board::GetTicks();
	if ( timeout > 0 ) {
		struct timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = 1000 * ( timeout % 1000 );
		setsockopt( mSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval) );
	}

	if ( mPortType == UDP or mPortType == UDPLite ) {
		uint32_t fromsize = sizeof( mClientSin );
		ret = recvfrom( mSocket, buf, len, 0, (SOCKADDR *)&mClientSin, &fromsize );
	} else {
		ret = recv( mClientSocket, buf, len, MSG_NOSIGNAL );
	}

	if ( ret <= 0 ) {
		if ( ( Board::GetTicks() - timebase >= timeout * 1000ULL ) or errno == 11 ) {
			return TIMEOUT;
		}
		gDebug() << "UDP disconnected ( " << ret << " : " << errno << ", " << strerror( errno ) << " )";
		mConnected = false;
		return -1;
	}

	return ret;
}


SyncReturn Socket::Write( const void* buf, uint32_t len, bool ack, int timeout )
{
	if ( !mConnected ) {
		return -1;
	}

	int ret = 0;

	if ( mPortType == UDP or mPortType == UDPLite ) {
		if ( mBroadcast ) {
			mClientSin.sin_family = AF_INET;
			mClientSin.sin_port = htons( mPort );
			mClientSin.sin_addr.s_addr = inet_addr( "192.168.32.255" );
		}
		if ( mClientSin.sin_family == AF_UNSPEC ) {
			return 0;
		}
		uint32_t sendsize = sizeof( mClientSin );
		uint32_t pos = 0;
		while ( pos < len and ret >= 0 ) {
			uint32_t sz = std::min( len - pos, 65000U );
			int r = sendto( mSocket, (uint8_t*)buf + pos, sz, 0, (SOCKADDR *)&mClientSin, sendsize );
			if ( r < 0 ) {
				ret = r;
				break;
			}
			pos += r;
			ret += r;
		}
	} else {
		ret = send( mClientSocket, buf, len, MSG_NOSIGNAL );
	}

	if ( ret <= 0 and ( errno == EAGAIN or errno == -EAGAIN ) ) {
		return 0;
	}

	if ( ret < 0 and mPortType != UDP and mPortType != UDPLite ) {
		gDebug() << "TCP disconnected";
		mConnected = false;
		return -1;
	}
	return ret;
}


LuaValue Socket::infos() const
{
	LuaValue ret;

	ret[ "Port" ] = (int)mPort;
	if ( mPortType == TCP ) {
		ret[ "Port Type" ] = "TCP";
	} else if ( mPortType == UDP ) {
		ret[ "Port Type" ] = "UDP";
	} else if ( mPortType == UDPLite ) {
		ret[ "Port Type" ] = "UDPLite";
	}
	ret[ "Broadcast" ] = mBroadcast ? "true" : "false";
	ret[ "Timeout" ] = mTimeout;

	return ret;
}

#endif // ( BUILD_SOCKET == 1 )
