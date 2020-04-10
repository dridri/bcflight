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
#include <fstream>
#include <algorithm>
#include <string.h>
#include "Debug.h"
#include "Config.h"
#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <Motor.h>
#include <Link.h>

extern "C" uint32_t _mem_usage();

Config::Config( const string& filename, const string& settings_filename )
	: mFilename( filename )
	, mSettingsFilename( settings_filename )
	, L( nullptr )
{
	gDebug() << "step 1 : " << _mem_usage() << "\n";

	gDebug() << LUA_RELEASE << "\n";
	L = luaL_newstate();
	gDebug() << "step 2 : " << _mem_usage() << "\n";

// 	luaL_openlibs( L );
	static const luaL_Reg lualibs[] = {
		{ "", luaopen_base },
		{ LUA_TABLIBNAME, luaopen_table },
		{ LUA_STRLIBNAME, luaopen_string },
		{ LUA_MATHLIBNAME, luaopen_math },
		{ nullptr, nullptr }
	};

	const luaL_Reg* lib = lualibs;
	for (; lib->func; lib++ ) {
		lua_pushcfunction( L, lib->func );
		lua_pushstring( L, lib->name );
		lua_call( L, 1, 0 );
	}

	gDebug() << "step 3 : " << _mem_usage() << "\n";

	Reload();

	gDebug() << "step 4 : " << _mem_usage() << "\n";
}


Config::~Config()
{
	lua_close(L);
}


string Config::ReadFile()
{
	string ret = "";
	ifstream file( mFilename );

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


void Config::WriteFile( const string& content )
{
	ofstream file( mFilename );

	if ( file.is_open() ) {
		file.write( content.c_str(), content.length() );
		file.close();
	}
}


string Config::String( const string& name, const string& def )
{
	gDebug() << "Config::string( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	string ret = lua_tolstring( L, -1, nullptr );
	Debug() << " => " << ret << "\n";
	return ret;
}


int Config::Integer( const string& name, int def )
{
	gDebug() << "Config::integer( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	int ret = lua_tointeger( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


float Config::Number( const string& name, float def )
{
	gDebug() << "Config::number( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	float ret = lua_tonumber( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


bool Config::Boolean( const string& name, bool def )
{
	gDebug() << "Config::boolean( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	bool ret = lua_toboolean( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


vector<int> Config::IntegerArray( const string& name )
{
	gDebug() << "Config::integerArray( " << name << " )";
	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return vector<int>();
	}

	vector<int> ret;
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
	Debug() << " => Ok\n";
	return ret;
}


int Config::LocateValue( const string& _name )
{
	const char* name = _name.c_str();

	if ( strchr( name, '.' ) == nullptr and strchr( name, '[' ) == nullptr ) {
#ifdef LUA_GLOBALSINDEX
		lua_getfield( L, LUA_GLOBALSINDEX, name );
#else
		lua_rawgeti( L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS );
		lua_getfield( L, -1, name );
		lua_copy( L, -1, -2 );
		lua_pop( L, 1 );
#endif
		if ( lua_type( L, -1 ) == LUA_TNIL ) {
			return -1;
		}
	} else {
		char tmp[128];
		int i=0, j, k;
		bool in_table = false;
		for ( i = 0, j = 0, k = 0; name[i]; i++ ) {
			if ( name[i] == '.' or name[i] == '[' or name[i] == ']' ) {
				tmp[j] = 0;
				if ( strlen(tmp) == 0 ) {
					j = 0;
					k++;
					continue;
				}
				if ( name[i] == '[' ) {
					in_table = true;
				}
				if ( in_table and name[i] == ']' and lua_istable( L, -1 ) ) {
					if ( tmp[0] >= '0' and tmp[0] <= '9' ) {
						lua_rawgeti( L, -1, atoi(tmp) );
					} else {
						lua_getfield( L, -1, tmp );
					}
				} else {
					if ( k == 0 ) {
#ifdef LUA_GLOBALSINDEX
						lua_getfield( L, LUA_GLOBALSINDEX, tmp );
#else
						lua_rawgeti( L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS );
						lua_getfield( L, -1, tmp );
						lua_copy( L, -1, -2 );
						lua_pop( L, 1 );
#endif
					} else {
						lua_getfield( L, -1, tmp );
					}
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
		if ( tmp[0] != 0 ) {
			lua_getfield( L, -1, tmp );
			if ( lua_type( L, -1 ) == LUA_TNIL ) {
				return -1;
			}
		}
	}

	return 0;
}


int Config::ArrayLength( const string& name )
{
	if ( LocateValue( name ) < 0 ) {
		return -1;
	}

	int ret = -1;

	if ( lua_istable( L, -1 ) ) {
		ret = 0;
		size_t len = lua_objlen( L, -1 );
		if ( len > 0 ) {
			ret = len;
		} else {
			lua_pushnil( L );
			while( lua_next( L, -2 ) != 0 ) {
				ret++;
				lua_pop( L, 1 );
			}
		}
	}

	return ret;
}


string Config::DumpVariable( const string& name, int index, int indent )
{
	stringstream ret;

	for ( int i = 0; i < indent; i++ ) {
		ret << "    ";
	}
	if ( name != "" ) {
		ret << name << " = ";
	}

	if ( indent == 0 ) {
		LocateValue( name );
	}

	if ( lua_isnil( L, index ) ) {
		ret << "nil";
	} else if ( lua_isnumber( L, index ) ) {
		ret << lua_tonumber( L, index );
	} else if ( lua_isboolean( L, index ) ) {
		ret << ( lua_toboolean( L, index ) ? "true" : "false" );
	} else if ( lua_isstring( L, index ) ) {
		ret << "\"" << lua_tostring( L, index ) << "\"";
	} else if ( lua_iscfunction( L, index ) ) {
		ret << "C-function()";
	} else if ( lua_isuserdata( L, index ) ) {
		ret << "__userdata__";
	} else if ( lua_istable( L, index ) ) {
		ret << "{\n\r";
		size_t len = lua_objlen( L, index );
		if ( len > 0 ) {
			for ( size_t i = 1; i <= len; i++ ) {
				lua_rawgeti( L, index, i );
				ret << DumpVariable( "", -1, indent + 1 );
				lua_pop( L, 1 );
				ret << ",\n\r";
			}
		} else {
			lua_pushnil( L );
			while( lua_next( L, -2 ) != 0 ) {
				string key = lua_tostring( L, index-1 );
				if ( lua_isnumber( L, index - 1 ) ) {
					key = "[" + key + "]";
				}
				if ( key != "lens_shading" ) {
					ret << DumpVariable( key, index, indent + 1 );
				} else {
					for ( int i = 0; i < indent + 1; i++ ) {
						ret << "    ";
					}
					ret << key << " = {...}";
				}
				lua_pop( L, 1 );
				ret << ",\n\r";
			}
		}
		for ( int i = 0; i < indent; i++ ) {
			ret << "    ";
		}
		ret << "}";
	} else {
		ret << "__unknown__";
	}

	if ( indent == 0 ) {
		ret << "\n\r";
	}
	return ret.str();
}


void Config::Execute( const string& code )
{
	fDebug( code );
	luaL_dostring( L, code.c_str() );
}


static inline string& ltrim( string& s, const char* t = " \t\n\r\f\v" )
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}


static inline string& rtrim( string& s, const char* t = " \t\n\r\f\v" )
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}


static inline string& trim( string& s, const char* t = " \t\n\r\f\v" )
{
	return ltrim(rtrim(s, t), t);
}


#define LUAdostring( s ) gDebug() << "dostring : '" << s << "'\n"; luaL_dostring( L, string(s).c_str() )


void Config::Reload()
{
	gDebug() << "Reload 1 : " << _mem_usage() << "\n";
	LUAdostring( "board = { type = \"" + string( BOARD ) + "\" }" );
	LUAdostring( "frame = { motors = {} }" );
	LUAdostring( "battery = {}" );
	LUAdostring( "camera = {}" );
	LUAdostring( "hud = {}" );
	LUAdostring( "microphone = {}" );
	LUAdostring( "controller = {}" );
	LUAdostring( "stabilizer = { loop_time = 2000 }" );
	LUAdostring( "sensors_map_i2c = {}" );
	LUAdostring( "accelerometers = {}" );
	LUAdostring( "gyroscopes = {}" );
	LUAdostring( "magnetometers = {}" );
	LUAdostring( "altimeters = {}" );
	LUAdostring( "GPSes = {}" );
	LUAdostring( "user_sensors = {}" );
	LUAdostring( "function RegisterSensor( name, params ) user_sensors[name] = params ; return params end" );
	LUAdostring( "function Vector( x, y, z, w ) return { x = x, y = y, z = z, w = w } end" );
	LUAdostring( "function Buzzer( params ) params.type = \"Buzzer\" ; return params end" ); // TODO : remove this
	LUAdostring( "function Voltmeter( params ) params.type = \"Voltemeter\" ; return params end" );
	gDebug() << "Reload 2 : " << _mem_usage() << "\n";

#ifdef BUILD_motors
	for ( auto motor : Motor::knownMotors() ) {
		LUAdostring( "function " + motor.first + "( params ) params.motor_type = \"" + motor.first + "\" ; return params end" );
	}
#endif
	gDebug() << "Reload 3 : " << _mem_usage() << "\n";
#ifdef BUILD_links
	for ( auto link : Link::knownLinks() ) {
		LUAdostring( "function " + link.first + "( params ) params.link_type = \"" + link.first + "\" ; return params end" );
	}
#endif
	gDebug() << "Reload 4 : " << _mem_usage() << "\n";
#ifdef BUILD_sensors
	for ( const Sensor::Device& device : Sensor::KnownDevices() ) {
		LUAdostring( "function " + string(device.name) + "( params ) params.sensor_type = \"" + string(device.name) + "\" ; return params end" );
	}
#endif
	gDebug() << "Reload 5 : " << _mem_usage() << "\n";


	if ( mFilename != "" ) {
		luaL_loadfile( L, mFilename.c_str() );
		int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );
		if ( ret != 0 ) {
			lua_Debug ar;
			lua_getstack(L, 1, &ar);
			lua_getinfo(L, "nSl", &ar);
			gDebug() << "Lua : Error while executing file \"" << mFilename << "\" : " << ar.currentline << " : \"" << lua_tostring( L, -1 ) << "\"\n";
			return;
		}
	}

	if ( mSettingsFilename != "" ) {
		luaL_loadfile( L, mSettingsFilename.c_str() );
		int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );
		if ( ret != 0 ) {
			lua_Debug ar;
			lua_getstack(L, 1, &ar);
			lua_getinfo(L, "nSl", &ar);
			gDebug() << "Lua : Error while executing file \"" << mSettingsFilename << "\" : " << ar.currentline << " : \"" << lua_tostring( L, -1 ) << "\"\n";
		}
	
		string line;
		string key;
		string value;
		ifstream file( mSettingsFilename, fstream::in );
		if ( file.is_open() ) {
			while ( getline( file, line ) ) {
				key = line.substr( 0, line.find( "=" ) );
				value = line.substr( line.find( "=" ) + 1 );
				key = trim( key );
				value = trim( value );
				mSettings[ key ] = value;
				gDebug() << "mSettings[\"" << key << "\"] = '" << mSettings[ key ] << "'\n";
			}
		}
	}

#ifdef BUILD_sensors
	list< string > user_sensors;
	LocateValue( "user_sensors" );
	lua_pushnil( L );
	while( lua_next( L, -2 ) != 0 ) {
		string key = lua_tostring( L, -2 );
		user_sensors.emplace_back( key );
		lua_pop( L, 1 );
	}
	for ( string s : user_sensors ) {
		Sensor::RegisterDevice( s, this, "user_sensors." + s );
	}
#endif
}


void Config::Apply()
{
#ifdef BUILD_sensors
	list< string > accelerometers;
	list< string > gyroscopes;
	list< string > magnetometers;

	lua_getglobal( L, "accelerometers" );
	lua_pushnil( L );
	while ( lua_next( L, -2 ) ) {
		accelerometers.emplace_back( lua_tolstring( L, -2, nullptr ) );
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	lua_getglobal( L, "gyroscopes" );
	lua_pushnil( L );
	while ( lua_next( L, -2 ) ) {
		gyroscopes.emplace_back( lua_tolstring( L, -2, nullptr ) );
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	lua_getglobal( L, "magnetometers" );
	lua_pushnil( L );
	while ( lua_next( L, -2 ) ) {
		magnetometers.emplace_back( lua_tolstring( L, -2, nullptr ) );
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	for ( auto it : gyroscopes ) {
		Gyroscope* gyro = Sensor::gyroscope( it );
		if ( gyro ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = Integer( "gyroscopes." + it + ".axis_swap.x" );
			swap[1] = Integer( "gyroscopes." + it + ".axis_swap.y" );
			swap[2] = Integer( "gyroscopes." + it + ".axis_swap.z" );
			gyro->setAxisSwap( swap );
		}
	}
	for ( auto it : accelerometers ) {
		Accelerometer* accel = Sensor::accelerometer( it );
		if ( accel ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = Integer( "accelerometers." + it + ".axis_swap.x" );
			swap[1] = Integer( "accelerometers." + it + ".axis_swap.y" );
			swap[2] = Integer( "accelerometers." + it + ".axis_swap.z" );
			accel->setAxisSwap( swap );
		}
	}
	for ( auto it : magnetometers ) {
		Magnetometer* magn = Sensor::magnetometer( it );
		if ( magn ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = Integer( "magnetometers." + it + ".axis_swap.x" );
			swap[1] = Integer( "magnetometers." + it + ".axis_swap.y" );
			swap[2] = Integer( "magnetometers." + it + ".axis_swap.z" );
			magn->setAxisSwap( swap );
		}
	}
#endif // BUILD_sensors
}


void Config::setBoolean( const string& name, const bool v )
{
	mSettings[name] = ( v ? "true" : "false" );
	LUAdostring( name + "=" + mSettings[name] );
}


void Config::setInteger( const string& name, const int v )
{
	mSettings[name] = to_string(v);
	LUAdostring( name + "=" + mSettings[name] );
}


void Config::setNumber( const string& name, const float v )
{
	mSettings[name] = to_string(v);
	LUAdostring( name + "=" + mSettings[name] );
}


void Config::setString( const string& name, const string& v )
{
	mSettings[name] = v;
	LUAdostring( name + "=" + v );
}


void Config::Save()
{
	ofstream file( mSettingsFilename );
	string str;

	if ( file.is_open() ) {
		for ( pair< string, string > setting : mSettings ) {
			str = setting.first + " = " + setting.second + "\n";
			gDebug() << "Saving setting  : " << str;
			file.write( str.c_str(), str.length() );
		}
		file.close();
	}
}
