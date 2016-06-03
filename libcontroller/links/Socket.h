#ifndef SOCKET_H
#define SOCKET_H

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
