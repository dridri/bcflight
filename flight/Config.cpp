#include <list>
#include <string.h>
#include "Debug.h"
#include "Config.h"
#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>

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


bool Config::boolean( const std::string& name )
{
	Debug() << "Config::boolean( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return 0;
	}

	bool ret = lua_toboolean( L, -1 );
	Debug() << " => " << ret << "\n";
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
			std::string key = lua_tostring( L, index-1 );
			if ( lua_isnumber( L, index - 1 ) ) {
				key = "[" + key + "]";
			}
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
	luaL_dostring( L, "function RawWifi( params ) params.link_type = \"RawWifi\" ; params.device = \"wlan0\" ; if params.blocking == nil then params.blocking = true end ; if params.retries == nil then params.retries = 2 end ; return params end" );
	luaL_dostring( L, "function Voltmeter( params ) params.sensor_type = \"Voltmeter\" ; return params end" );
	luaL_dostring( L, ( "board = { type = \"" + std::string( BOARD ) + "\" }" ).c_str() );
	luaL_dostring( L, "frame = { motors = { front_left = {}, front_right = {}, rear_left = {}, rear_right = {} } }" );
	luaL_dostring( L, "battery = {}" );
	luaL_dostring( L, "camera = {}" );
	luaL_dostring( L, "controller = {}" );
	luaL_dostring( L, "stabilizer = { loop_time = 2500 }" );
	luaL_dostring( L, "accelerometers = {}" );
	luaL_dostring( L, "gyroscopes = {}" );
	luaL_dostring( L, "magnetometers = {}" );
	luaL_dostring( L, "altimeters = {}" );
	luaL_dostring( L, "GPSes = {}" );
	luaL_dostring( L, "user_sensors = {}" );
	luaL_dostring( L, "function RegisterSensor( name, params ) user_sensors[name] = params ; return params end" );
	luaL_loadfile( L, mFilename.c_str() );
	int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );

	if ( ret != 0 ) {
		gDebug() << "Lua : Error while executing file \"" << mFilename << "\" : \"" << lua_tostring( L, -1 ) << "\"\n";
		return;
	}

	std::list< std::string > user_sensors;
	LocateValue( "user_sensors" );
	lua_pushnil( L );
	while( lua_next( L, -2 ) != 0 ) {
		std::string key = lua_tostring( L, -2 );
		user_sensors.emplace_back( key );
		lua_pop( L, 1 );
	}

	for ( std::string s : user_sensors ) {
		Sensor::RegisterDevice( s, this, "user_sensors." + s );
	}
}


void Config::Apply()
{
	std::list< std::string > accelerometers;
	std::list< std::string > gyroscopes;
	std::list< std::string > magnetometers;

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
			swap[0] = integer( "gyroscopes." + it + ".axis_swap.x" );
			swap[1] = integer( "gyroscopes." + it + ".axis_swap.y" );
			swap[2] = integer( "gyroscopes." + it + ".axis_swap.z" );
			gyro->setAxisSwap( swap );
		}
	}
	for ( auto it : accelerometers ) {
		Accelerometer* accel = Sensor::accelerometer( it );
		if ( accel ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = integer( "accelerometers." + it + ".axis_swap.x" );
			swap[1] = integer( "accelerometers." + it + ".axis_swap.y" );
			swap[2] = integer( "accelerometers." + it + ".axis_swap.z" );
			accel->setAxisSwap( swap );
		}
	}
	for ( auto it : magnetometers ) {
		Magnetometer* magn = Sensor::magnetometer( it );
		if ( magn ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = integer( "magnetometers." + it + ".axis_swap.x" );
			swap[1] = integer( "magnetometers." + it + ".axis_swap.y" );
			swap[2] = integer( "magnetometers." + it + ".axis_swap.z" );
			magn->setAxisSwap( swap );
		}
	}
}


void Config::Save()
{
	// TODO
}
