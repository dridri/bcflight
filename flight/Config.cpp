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
	if ( LocateValue( name ) < 0 ) {
		return "";
	}
	return lua_tolstring( L, -1, nullptr );
}


int Config::integer( const std::string& name )
{
	if ( LocateValue( name ) < 0 ) {
		return 0;
	}
	return lua_tointeger( L, -1 );
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
	luaL_loadfile( L, mFilename.c_str() );
	lua_pcall( L, 0, LUA_MULTRET, 0 );
}


void Config::Save()
{
	// TODO
}
