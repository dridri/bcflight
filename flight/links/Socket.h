#ifndef SOCKET_H
#define SOCKET_H

#if ( BUILD_SOCKET == 1 )

#include <netinet/in.h>
#include "Link.h"

class Socket : public Link
{
public:
	typedef enum {
		TCP,
		UDP,
		UDPLite
	} PortType;

	Socket( uint16_t port, PortType type = TCP, bool broadcast = false );
	virtual ~Socket();

	int Connect();
	int setBlocking( bool blocking );
	int Read( void* buf, uint32_t len, int timeout = 0 );
	int ReadFloat( float* f );
	int ReadU32( uint32_t* v );
	int Write( void* buf, uint32_t len, int timeout = 0 );
	int WriteU32( uint32_t v );
	int WriteFloat( float v );
	int WriteString( const std::string& s );
	int WriteString( const char* fmt, ... );

protected:
	uint16_t mPort;
	PortType mPortType;
	bool mBroadcast;
	int mSocket;
	struct sockaddr_in mSin;
	int mClientSocket;
	struct sockaddr_in mClientSin;
};

#endif // ( BUILD_SOCKET == 1 )

#endif // SOCKET_H
