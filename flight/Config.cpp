#include <string.h>
#include "Debug.h"
#include "Config.h"

Config::Config( const std::string& filename )
	: mFilename( filename )
	, L( nullptr )
{
	L = luaL_newstate();
	luaL_openlibs( L );

	Reload();
}


Config::~Config()
{
	lua_close(L);
}


std::string Config::string( const std::string& name )
{
	Debug() << "Config::string( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return "";
	}

	std::string ret = lua_tolstring( L, -1, nullptr );
	Debug() << " => " << ret << "\n";
	return ret;
}


int Config::integer( const std::string& name )
{
	Debug() << "Config::integer( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return 0;
	}

	int ret = lua_tointeger( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


float Config::number( const std::string& name )
{
	Debug() << "Config::number( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return 0;
	}

	float ret = lua_tonumber( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


int Config::LocateValue( const std::string& _name )
{
	const char* name = _name.c_str();

	if ( not strchr( name, '.' ) ) {
		lua_getfield( L, LUA_GLOBALSINDEX, name );
	} else {
		char tmp[128];
		int i, j, k;
		for ( i = 0, j = 0, k = 0; name[i]; i++ ) {
			if ( name[i] == '.' ) {
				tmp[j] = 0;
				if ( k == 0 ) {
					lua_getfield( L, LUA_GLOBALSINDEX, tmp );
				} else {
					lua_getfield( L, -1, tmp );
				}
				memset (tmp, 0, sizeof( tmp ) );
				j = 0;
				k++;
			} else {
				tmp[j] = name[i];
				j++;
			}
		}
		tmp[j] = 0;
		lua_getfield( L, -1, tmp);
	}

	return 0;
}


void Config::Reload()
{
	luaL_dostring( L, "function Vector( x, y, z ) return { x = x, y = y, z = z } end" );
	luaL_loadfile( L, mFilename.c_str() );
	int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );

	if ( ret != 0 ) {
		gDebug() << "Lua : Error while executing file \"" << mFilename << "\" : \"" << lua_tostring( L, -1 ) << "\"\n";
	}
}


void Config::Save()
{
	// TODO
}
