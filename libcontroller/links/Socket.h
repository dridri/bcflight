#ifndef SOCKET_H
#define SOCKET_H

#if ( BUILD_SOCKET == 1 )

#include <netinet/in.h>
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

	Socket( uint16_t port, PortType type = TCP, bool broadcast = false );
	virtual ~Socket();

	int Connect();
	int setBlocking( bool blocking );

protected:
	int Read( void* buf, uint32_t len, int32_t timeout );
	int Write( const void* buf, uint32_t len, int32_t timeout );

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
