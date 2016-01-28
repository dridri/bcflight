#include "Link.h"
#include <Debug.h>
#include <Config.h>

std::map< std::string, std::function< Link* ( Config*, const std::string& ) > > Link::mKnownLinks;

Link::Link()
	: mConnected( false )
{
}


Link::~Link()
{
}


bool Link::isConnected() const
{
	return mConnected;
}


Link* Link::Create( Config* config, const std::string& lua_object )
{
	std::string type = config->string( lua_object + ".link_type" );

	if ( mKnownLinks.find( type ) != mKnownLinks.end() ) {
		return mKnownLinks[ type ]( config, lua_object );
	}

	gDebug() << "FATAL ERROR : Link type \"" << type << "\" not supported !\n";
	return nullptr;
} // -- Socket{ type = "TCP/UDP/UDPLite", port = port_number[, broadcast = true/false] } <= broadcast is false by default


void Link::RegisterLink( const std::string& name, std::function< Link* ( Config*, const std::string& ) > instanciate )
{
	mKnownLinks[ name ] = instanciate;
}
