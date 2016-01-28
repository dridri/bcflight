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


void Config::DumpVariable( const std::string& name, int index, int indent )
{
	for ( int i = 0; i < indent; i++ ) {
		Debug() << "    ";
	}
	Debug() << name << " = ";

	if ( indent == 0 ) {
		LocateValue( name );
	}

	if ( lua_isnil( L, index ) ) {
		Debug() << "nil";
	} else if ( lua_isnumber( L, index ) ) {
		Debug() << lua_tonumber( L, index );
	} else if ( lua_isboolean( L, index ) ) {
		Debug() << ( lua_toboolean( L, index ) ? "true" : "false" );
	} else if ( lua_isstring( L, index ) ) {
		Debug() << "\"" << lua_tostring( L, index ) << "\"";
	} else if ( lua_iscfunction( L, index ) ) {
		Debug() << "C-function()";
	} else if ( lua_isuserdata( L, index ) ) {
		Debug() << "__userdata__";
	} else if ( lua_istable( L, index ) ) {
		Debug() << "{\n";
		lua_pushnil( L );
		while( lua_next( L, -2 ) != 0 ) {
			const char* key = lua_tostring( L, index-1 );
			DumpVariable( key, index, indent + 1 );
			lua_pop( L, 1 );
			Debug() << ",\n";
		}
		for ( int i = 0; i < indent; i++ ) {
			Debug() << "    ";
		}
		Debug() << "}";
	} else {
		Debug() << "__unknown__";
	}

	if ( indent == 0 ) {
		Debug() << "\n";
	}
}


void Config::Reload()
{
	luaL_dostring( L, "function Vector( x, y, z, w ) return { x = x, y = y, z = z, w = w } end" );
	luaL_dostring( L, "function Socket( params ) params.link_type = \"Socket\" ; return params end" );
	luaL_dostring( L, "function Voltmeter( params ) params.sensor_type = \"Voltmeter\" ; return params end" );
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
