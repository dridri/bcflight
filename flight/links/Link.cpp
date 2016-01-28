#include "Link.h"
#include <Debug.h>
#include <Config.h>

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
	// TODO : iterate over all available link types, then return FoundLink::Create( config, lua_object )

	gDebug() << "FATAL ERROR : Link type \"" << type << "\" not supported !\n";
	return nullptr;
}

// -- Socket{ type = "TCP/UDP/UDPLite", port = port_number[, broadcast = true/false] } <= broadcast is false by default
