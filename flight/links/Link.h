#ifndef LINK_H
#define LINK_H

#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <map>
#include <functional>

class Config;

class Link
{
public:
	static Link* Create( Config* config, const std::string& lua_object );
	Link();
	virtual ~Link();

	bool isConnected() const;

	virtual int Connect() = 0;
	virtual int setBlocking( bool blocking ) = 0;
	virtual int Read( void* buf, uint32_t len, int timeout = 0 ) = 0;
	virtual int ReadU32( uint32_t* v ) = 0;
	virtual int ReadFloat( float* f ) = 0;
	virtual int Write( void* buf, uint32_t len, int timeout = 0 ) = 0;
	virtual int WriteU32( uint32_t v ) = 0;
	virtual int WriteFloat( float v ) = 0;
	virtual int WriteString( const std::string& s ) = 0;
	virtual int WriteString( const char* fmt, ... ) = 0;

protected:
	bool mConnected;

	static void RegisterLink( const std::string& name, std::function< Link* ( Config*, const std::string& ) > instanciate );
	static std::map< std::string, std::function< Link* ( Config*, const std::string& ) > > mKnownLinks;
};

#endif // LINK_H
