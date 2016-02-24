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
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#include "../Debug.h"
#include "Socket.h"
#include "../Config.h"


int Socket::flight_register( Main* main )
{
	RegisterLink( "Socket", &Socket::Instanciate );
	return 0;
}


Link* Socket::Instanciate( Config* config, const std::string& lua_object )
{
	PortType type = UDPLite; // Default to UDPLite

	std::string stype = config->string( lua_object + ".type" );
	if ( stype == "TCP" ) {
		type = TCP;
	} else if ( stype == "UDP" ) {
		type = UDP;
	} else if ( stype == "UDPLite" ) {
		type = UDPLite;
	} else {
		gDebug() << "FATAL ERROR : Unsupported Socket type \"" << stype << "\" !\n";
	}

	int port = config->integer( lua_object + ".port" );
	bool broadcast = config->boolean( lua_object + ".broadcast" );

	return new Socket( port, type, broadcast );
}


Socket::Socket( uint16_t port, PortType type, bool broadcast )
	: mPort( port )
	, mPortType( type )
	, mBroadcast( broadcast )
	, mSocket( -1 )
	, mClientSocket( -1 )
{
}


Socket::~Socket()
{
	if ( mConnected and mSocket >= 0 ) {
		shutdown( mSocket, 2 );
		closesocket( mSocket );
	}
}


int Socket::Connect()
{
	fDebug0();
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
		if ( bind( mSocket, (SOCKADDR*)&mSin, sizeof(mSin) ) < 0 ) {
			gDebug() << "Socket ( " << mPort << " ) error : " << strerror(errno) << "\n";
			return -1;
		}
	}

	if ( mPortType == TCP ) {
		int ret = listen( mSocket, 5 );
		int size = 0;
		if ( !ret ) {
			mClientSocket = accept( mSocket, (SOCKADDR*)&mClientSin, (socklen_t*)&size );
		} else {
			return -1;
		}
	} else if ( mPortType == UDP or mPortType == UDPLite ) {
		if ( not mBroadcast ) {
			uint32_t flag = 0;
			uint32_t fromsize = sizeof( mClientSin );
			int ret = recvfrom( mSocket, &flag, sizeof( flag ), 0, (SOCKADDR *)&mClientSin, &fromsize );
			if ( ret > 0 ) {
				flag = ntohl( flag );
				gDebug() << "flag : " << ntohl( flag ) << "\n";
				if ( flag != 0x12345678 ) {
					return -1;
				}
			} else {
				gDebug() << strerror( errno ) << "\n";
				return -1;
			}
		}
	}

	mConnected = true;
	return 0;
}


int Socket::setBlocking( bool blocking )
{
	int flags = fcntl( mSocket, F_GETFL, 0 );
	flags = blocking ? ( flags & ~O_NONBLOCK) : ( flags | O_NONBLOCK );
	return ( fcntl( mSocket, F_SETFL, flags ) == 0 );
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
		uint32_t fromsize = sizeof( mClientSin );
		ret = recvfrom( mSocket, buf, len, 0, (SOCKADDR *)&mClientSin, &fromsize );
		if ( ret <= 0 and errno != EAGAIN ) {
			gDebug() << "UDP disconnected ( " << ret << " : " << strerror( errno ) << " )\n";
			mConnected = false;
			return -1;
		}
// 		return -1;
	} else {
		ret = recv( mClientSocket, buf, len, MSG_NOSIGNAL );
		if ( ret <= 0 ) {
			gDebug() << "TCP disconnected\n";
			mConnected = false;
			return -1;
		}
	}

	return ret;
}


int Socket::ReadFloat( float* f )
{
	union {
		float f;
		uint32_t u;
	} d;
	int ret = Read( &d.u, sizeof( uint32_t ) );
	d.u = ntohl( d.u );
	*f = d.f;
	return ret;
}


int Socket::ReadU32( uint32_t* v )
{
	int ret = Read( v, sizeof( uint32_t ) );
	if ( ret == sizeof( uint32_t ) ) {
		*v = ntohl( *v );
	}
	return ret;
}


int Socket::Write( void* buf, uint32_t len, int timeout )
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
		uint32_t sendsize = sizeof( mClientSin );
		ret = sendto( mSocket, buf, len, 0, (SOCKADDR *)&mClientSin, sendsize );
	} else {
		ret = send( mClientSocket, buf, len, 0 );
	}

	if ( ret <= 0 and ( errno == EAGAIN or errno == -EAGAIN ) ) {
		return 0;
	}

	if ( ret < 0 and mPortType != UDP and mPortType != UDPLite ) {
		gDebug() << "TCP disconnected\n";
		mConnected = false;
		return -1;
	}
	return ret;
}


int Socket::WriteU32( uint32_t v )
{
	v = htonl( v );
	return Write( &v, sizeof(v) );
}


int Socket::WriteFloat( float v )
{
	union {
		float f;
		uint32_t u;
	} u;
	u.f = v;
	u.u = htonl( u.u );
	return Write( &u, sizeof( u ) );
}


int Socket::WriteString( const std::string& s )
{
	return Write( (void*)s.c_str(), s.length() );
}


int Socket::WriteString( const char* fmt, ... )
{
	va_list opt;
	char buff[1024] = "";
	va_start( opt, fmt );
	vsnprintf( buff, (size_t) sizeof(buff), fmt, opt );
	return Write( buff, strlen(buff) + 1 );
}

#endif // ( BUILD_SOCKET == 1 )
