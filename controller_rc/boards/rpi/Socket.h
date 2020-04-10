#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <netinet/in.h>

namespace rpi {

class Socket
{
public:
	typedef enum {
		Server,
		Client
	} SocketType;
	typedef enum {
		TCP,
		UDP,
		UDPLite
	} PortType;

	Socket();
	Socket( uint16_t port, PortType type = TCP );
	Socket( const std::string& host, uint16_t port, PortType type = TCP );
	~Socket();

	bool isConnected() const;
	int Connect();
	Socket* WaitClient();

	int Receive( void* buf, uint32_t len, int32_t timeout = -1 );
	int Send( const void* buf, uint32_t len, int32_t timeout = -1 );

protected:
	SocketType mSocketType;
	std::string mHost;
	uint16_t mPort;
	PortType mPortType;
	int mSocket;
	struct sockaddr_in mSin;
	struct sockaddr_in mClientSin;
};

} // namespace rpi

#endif // SOCKET_H
