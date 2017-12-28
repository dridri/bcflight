/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include "Config.h"

Config::Config( const std::string& filename )
	: mFilename( filename )
	, L( nullptr )
{
	L = luaL_newstate();
// 	luaL_openlibs( L );

	Reload();
}


Config::~Config()
{
	lua_close(L);
}


std::string Config::ReadFile()
{
	std::string ret = "";
	std::ifstream file( mFilename );

	if ( file.is_open() ) {
		file.seekg( 0, file.end );
		int length = file.tellg();
		char* buf = new char[ length + 1 ];
		file.seekg( 0, file.beg );
		file.read( buf, length );
		buf[length] = 0;
		ret = buf;
		delete buf;
		file.close();
	}

	return ret;
}


void Config::WriteFile( const std::string& content )
{
	std::ofstream file( mFilename );

	if ( file.is_open() ) {
		file.write( content.c_str(), content.length() );
		file.close();
	}
}


std::string Config::string( const std::string& name, const std::string& def )
{
	std::cout << "Config::string( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		std::cout << " => not found !\n";
		return def;
	}

	std::string ret = lua_tolstring( L, -1, nullptr );
	std::cout << " => " << ret << "\n";
	return ret;
}


int Config::integer( const std::string& name, int def )
{
	std::cout << "Config::integer( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		std::cout << " => not found !\n";
		return def;
	}

	int ret = lua_tointeger( L, -1 );
	std::cout << " => " << ret << "\n";
	return ret;
}


float Config::number( const std::string& name, float def )
{
	std::cout << "Config::number( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		std::cout << " => not found !\n";
		return def;
	}

	float ret = lua_tonumber( L, -1 );
	std::cout << " => " << ret << "\n";
	return ret;
}


bool Config::boolean( const std::string& name, bool def )
{
	std::cout << "Config::boolean( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		std::cout << " => not found !\n";
		return def;
	}

	bool ret = lua_toboolean( L, -1 );
	std::cout << " => " << ret << "\n";
	return ret;
}


std::vector<int> Config::integerArray( const std::string& name )
{
	std::cout << "Config::integerArray( " << name << " )";
	if ( LocateValue( name ) < 0 ) {
		std::cout << " => not found !\n";
		return std::vector<int>();
	}

	std::vector<int> ret;
	size_t len = lua_objlen( L, -1 );
	if ( len > 0 ) {
		for ( size_t i = 1; i <= len; i++ ) {
			lua_rawgeti( L, -1, i );
			int value = lua_tointeger( L, -1 );
			ret.emplace_back( value );
			lua_pop( L, 1 );
		}
	}
	lua_pop( L, 1 );
	std::cout << " => Ok\n";
	return ret;
}


int Config::LocateValue( const std::string& _name )
{
	const char* name = _name.c_str();

	if ( strchr( name, '.' ) == nullptr ) {
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
				if ( lua_type( L, -1 ) == LUA_TNIL ) {
					return -1;
				}
				memset( tmp, 0, sizeof( tmp ) );
				j = 0;
				k++;
			} else {
				tmp[j] = name[i];
				j++;
			}
		}
		tmp[j] = 0;
		lua_getfield( L, -1, tmp );
		if ( lua_type( L, -1 ) == LUA_TNIL ) {
			return -1;
		}
	}

	return 0;
}


void Config::DumpVariable( const std::string& name, int index, int indent )
{
	for ( int i = 0; i < indent; i++ ) {
		std::cout << "    ";
	}
	if ( name != "" ) {
		std::cout << name << " = ";
	}

	if ( indent == 0 ) {
		LocateValue( name );
	}

	if ( lua_isnil( L, index ) ) {
		std::cout << "nil";
	} else if ( lua_isnumber( L, index ) ) {
		std::cout << lua_tonumber( L, index );
	} else if ( lua_isboolean( L, index ) ) {
		std::cout << ( lua_toboolean( L, index ) ? "true" : "false" );
	} else if ( lua_isstring( L, index ) ) {
		std::cout << "\"" << lua_tostring( L, index ) << "\"";
	} else if ( lua_iscfunction( L, index ) ) {
		std::cout << "C-function()";
	} else if ( lua_isuserdata( L, index ) ) {
		std::cout << "__userdata__";
	} else if ( lua_istable( L, index ) ) {
		std::cout << "{\n";
		size_t len = lua_objlen( L, index );
		if ( len > 0 ) {
			for ( size_t i = 1; i <= len; i++ ) {
				lua_rawgeti( L, index, i );
				DumpVariable( "", -1, indent + 1 );
				lua_pop( L, 1 );
				std::cout << ",\n";
			}
		} else {
			lua_pushnil( L );
			while( lua_next( L, -2 ) != 0 ) {
				std::string key = lua_tostring( L, index-1 );
				if ( lua_isnumber( L, index - 1 ) ) {
					key = "[" + key + "]";
				}
				DumpVariable( key, index, indent + 1 );
				lua_pop( L, 1 );
				std::cout << ",\n";
			}
		}
		for ( int i = 0; i < indent; i++ ) {
			std::cout << "    ";
		}
		std::cout << "}";
	} else {
		std::cout << "__unknown__";
	}

	if ( indent == 0 ) {
		std::cout << "\n";
	}
}


void Config::Reload()
{
	luaL_dostring( L, "function Vector( x, y, z, w ) return { x = x, y = y, z = z, w = w } end" );
	luaL_dostring( L, "function Socket( params ) params.link_type = \"Socket\" ; return params end" );
	luaL_dostring( L, "function RF24( params ) params.link_type = \"nRF24L01\" ; return params end" );
	luaL_dostring( L, "function SX127x( params ) params.link_type = \"SX127x\" ; return params end" );
	luaL_dostring( L, "function RawWifi( params ) params.link_type = \"RawWifi\" ; if params.device == nil then params.device = \"wlan0\" end ; if params.blocking == nil then params.blocking = true end ; if params.retries == nil then params.retries = 2 end ; return params end" );
	luaL_dostring( L, "function Voltmeter( params ) params.sensor_type = \"Voltmeter\" ; return params end" );
	luaL_dostring( L, "function Buzzer( params ) params.type = \"Buzzer\" ; return params end" );
	luaL_dostring( L, "battery = {}" );
	luaL_dostring( L, "controller = {}" );
	luaL_dostring( L, "stream = {}" );
	luaL_dostring( L, "touchscreen = {}" );
	luaL_loadfile( L, mFilename.c_str() );
	int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );

	if ( ret != 0 ) {
		std::cout << "Lua : Error while executing file \"" << mFilename << "\" : \"" << lua_tostring( L, -1 ) << "\"\n";
		return;
	}
}


void Config::Save()
{
}


const bool Config::setting( const std::string& name, const bool def ) const
{
	if ( mSettings.count( name ) > 0 ) {
		return ( mSettings.at( name ) == "true" );
	}
	return def;
}


const int Config::setting( const std::string& name, const int def ) const
{
	if ( mSettings.count( name ) > 0 ) {
		return atoi( mSettings.at( name ).c_str() );
	}
	return def;
}


const float Config::setting( const std::string& name, const float def ) const
{
	if ( mSettings.count( name ) > 0 ) {
		return atof( mSettings.at( name ).c_str() );
	}
	return def;
}


const std::string& Config::setting( const std::string& name, const std::string& def ) const
{
	if ( mSettings.count( name ) > 0 ) {
		return mSettings.at( name );
	}
	return def;
}


void Config::setSetting( const std::string& name, const bool v )
{
	mSettings[ name ] = ( v ? "true" : "false" );
}


void Config::setSetting( const std::string& name, const int v )
{
	std::stringstream ss;
	ss << v;
	mSettings[ name ] = ss.str();
}


void Config::setSetting( const std::string& name, const float v )
{
	std::stringstream ss;
	ss << v;
	mSettings[ name ] = ss.str();
}


void Config::setSetting( const std::string& name, const std::string& v )
{
	mSettings[ name ] = v;
}


void Config::LoadSettings( const std::string& filename )
{
	std::string line;
	std::string key;
	std::string value;
	std::ifstream file( filename, std::ios_base::in );

	if ( file.is_open() ) {
		while ( std::getline( file, line ) ) {
			key = line.substr( 0, line.find( "=" ) );
			value = line.substr( line.find( "=" ) + 1 );
			mSettings[ key ] = value;
			std::cout << "mSettings[ " << key << " ] = '" << mSettings[ key ] << "'\n";
		}
	}
}


void Config::SaveSettings( const std::string& filename )
{
	std::stringstream ss;
	std::ofstream file( filename, std::ios_base::out );

	if ( file.is_open() ) {
		for ( auto it : mSettings ) {
			ss.str( "" );
			ss << it.first << "=" << it.second << "\n";
			std::cout << "Write : " << ss.str();
			file.write( ss.str().c_str(), ss.str().length() );
		}
	}
}
