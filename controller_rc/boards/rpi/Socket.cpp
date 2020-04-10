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
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#include "Socket.h"
#include <iostream>

#define UDPLITE_SEND_CSCOV   10 /* sender partial coverage (as sent)      */
#define UDPLITE_RECV_CSCOV   11 /* receiver partial coverage (threshold ) */

rpi::Socket::Socket()
	: mSocketType( Client )
	, mPort( 0 )
	, mPortType( TCP )
	, mSocket( -1 )
{
}


rpi::Socket::Socket( uint16_t port, PortType type )
	: mSocketType( Server )
	, mPort( port )
	, mPortType( type )
	, mSocket( -1 )
{
}


rpi::Socket::Socket( const std::string& host, uint16_t port, PortType type )
	: mSocketType( Client )
	, mHost( host )
	, mPort( port )
	, mPortType( type )
	, mSocket( -1 )
{
}


rpi::Socket::~Socket()
{
	if ( mSocket >= 0 ) {
		shutdown( mSocket, 2 );
		closesocket( mSocket );
	}
}


bool rpi::Socket::isConnected() const
{
	return ( mSocket >= 0 );
}


int rpi::Socket::Connect()
{
	if ( mSocket < 0 ) {
		int type = ( mPortType == UDP or mPortType == UDPLite ) ? SOCK_DGRAM : SOCK_STREAM;
		int proto = ( mPortType == UDPLite ) ? IPPROTO_UDPLITE : ( ( mPortType == UDP ) ? IPPROTO_UDP : 0 );

		mSocket = socket( AF_INET, type, proto );
		int option = 1;
		setsockopt( mSocket, SOL_SOCKET, ( 15/*SO_REUSEPORT*/ | SO_REUSEADDR ), (char*)&option, sizeof( option ) );
		if ( mPortType == TCP ) {
			int flag = 1;
			setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int) );
		}
		if ( mPortType == UDPLite ) {
			uint16_t checksum_coverage = 8;
			setsockopt( mSocket, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV, &checksum_coverage, sizeof(checksum_coverage) );
			setsockopt( mSocket, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV, &checksum_coverage, sizeof(checksum_coverage) );
		}
		if ( mSocketType == Server ) {
			char myname[256];
			gethostname( myname, sizeof(myname) );
			memset( &mSin, 0, sizeof( mSin ) );
			mSin.sin_addr.s_addr = htonl( INADDR_ANY );
			mSin.sin_family = AF_INET;
			mSin.sin_port = htons( mPort );
			if ( bind( mSocket, (SOCKADDR*)&mSin, sizeof(mSin) ) < 0 ) {
				std::cout << "rpi::Socket ( " << mPort << " ) error : " << strerror(errno) << "\n";
				close( mSocket );
				return -1;
			}
		}
	}

	if ( mPortType == TCP ) {
		if ( mSocketType == Server ) {
		} else {
			memset( &mSin, 0, sizeof( mSin ) );
			struct hostent* hp = gethostbyname( mHost.c_str() );
			if ( hp and hp->h_addr_list and hp->h_addr_list[0] ) {
				mSin.sin_addr = *(IN_ADDR *) hp->h_addr;
			} else {
				mSin.sin_addr.s_addr = inet_addr( mHost.c_str() );
			}
			mSin.sin_family = AF_INET;
			mSin.sin_port = htons( mPort );
			if ( connect( mSocket, (SOCKADDR*)&mSin, sizeof(mSin) ) < 0 ) {
				std::cout << "rpi::Socket connect to " << mHost << ":" << mPort << " error : " << strerror(errno) << "\n";
				close( mSocket );
				mSocket = -1;
				return -1;
			}
		}
	} else if ( mPortType == UDP or mPortType == UDPLite ) {
		// TODO
	}

	return 0;
}


rpi::Socket* rpi::Socket::WaitClient()
{
	if ( mSocketType == Server and mPortType == TCP ) {
		int clientFD;
		struct sockaddr_in clientSin;
		int size = 0;

		int ret = listen( mSocket, 5 );
		if ( !ret ) {
			clientFD = accept( mSocket, (SOCKADDR*)&clientSin, (socklen_t*)&size );
			printf( "rpi::Socket::WaitClient clientFD : %d, %d, %s\n", ret, errno, strerror(errno) );
			if ( clientFD >= 0 ) {
				rpi::Socket* client = new rpi::Socket();
				client->mSocket = clientFD;
				client->mSin = clientSin;
				return client;
			}
		} else {
			printf( "rpi::Socket::WaitClient err %d, %d, %s\n", ret, errno, strerror(errno) );
			close( mSocket );
		}
	}

	return nullptr;
}


int rpi::Socket::Receive( void* buf, uint32_t len, int timeout )
{
	int ret = 0;
	memset( buf, 0, len );

	if ( mPortType == UDP or mPortType == UDPLite ) {
		uint32_t fromsize = sizeof( mClientSin );
		ret = recvfrom( mSocket, buf, len, 0, (SOCKADDR *)&mClientSin, &fromsize );
	} else {
		ret = recv( mSocket, buf, len, MSG_NOSIGNAL );
	}

	if ( ret <= 0 ) {
		if ( errno == 11 ) {
			return -11;
		}
		std::cout << "rpi::Socket on port " << mPort << " disconnected ( " << ret << " : " << errno << ", " << strerror( errno ) << " )\n";
		close( mSocket );
		mSocket = -1;
		return -1;
	}

	return ret;
}


int rpi::Socket::Send( const void* buf, uint32_t len, int timeout )
{
	int ret = 0;

	if ( mPortType == UDP or mPortType == UDPLite ) {
		uint32_t sendsize = sizeof( mClientSin );
		ret = sendto( mSocket, buf, len, 0, (SOCKADDR *)&mClientSin, sendsize );
	} else {
		ret = send( mSocket, buf, len, MSG_NOSIGNAL );
	}

	if ( ret <= 0 and ( errno == EAGAIN or errno == -EAGAIN ) ) {
		return 0;
	}

	if ( ret < 0 and mPortType != UDP and mPortType != UDPLite ) {
		std::cout << "rpi::Socket on port " << mPort << " disconnected\n";
		close( mSocket );
		mSocket = -1;
		return -1;
	}
	return ret;
}
