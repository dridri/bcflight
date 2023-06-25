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
extern "C" void lua_init( lua_State* L );

Config::Config( const string& filename, const string& settings_filename )
	: mFilename( filename )
	, mSettingsFilename( settings_filename )
	, mLua( nullptr )
{
	gDebug() << "step 1 : " << _mem_usage();

	gDebug() << LUA_RELEASE;
	// L = luaL_newstate();
	mLua = new Lua();
	lua_init( mLua->state() );
	gDebug() << "step 2 : " << _mem_usage();
/*
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
*/
}


Config::~Config()
{
	// lua_close(L);
	if ( mLua ) {
		delete mLua;
	}
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
	Debug dbg(Debug::Verbose);
	dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top(name);

	LuaValue v = mLua->value( name );
	if ( v.type() != LuaValue::String ) {
		dbg << " => not found !";
		return def;
	}

	string ret = v.toString();
	dbg << " => " << ret << "";
	return ret;
}


int Config::Integer( const string& name, int def )
{
	Debug dbg(Debug::Verbose);
	dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top(name);

	LuaValue v = mLua->value( name );
	if ( v.type() != LuaValue::Integer ) {
		dbg << " => not found !";
		return def;
	}

	int ret = v.toInteger();
	dbg << " => " << ret << "";
	return ret;
}


float Config::Number( const string& name, float def )
{
	Debug dbg(Debug::Verbose);
	dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top(name);

	LuaValue v = mLua->value( name );
	if ( v.type() != LuaValue::Number and v.type() != LuaValue::Integer ) {
		dbg << " => not found !";
		return def;
	}

	float ret = v.toNumber();
	dbg << " => " << ret << "";
	return ret;
}


bool Config::Boolean( const string& name, bool def )
{
	Debug dbg(Debug::Verbose);
	dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top(name);

	LuaValue v = mLua->value( name );
	if ( v.type() != LuaValue::Boolean ) {
		dbg << " => not found !";
		return def;
	}

	bool ret = v.toBoolean();
	dbg << " => " << ret << "";
	return ret;
}


void* Config::Object( const string& name, void* def )
{
	Debug dbg(Debug::Verbose);
	dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top(name);

	LuaValue v = mLua->value( name );
	if ( v.type() != LuaValue::UserData ) {
		dbg << " => not found !";
		return def;
	}

	void* ret = v.toUserData();
	dbg << " => " << ret << "";
	return ret;
}


vector<int> Config::IntegerArray( const string& name )
{
	Debug dbg(Debug::Verbose);
	dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top(name);

	LuaValue v = mLua->value( name );
	if ( v.type() != LuaValue::Table ) {
		dbg << " => not found !";
		return vector<int>();
	}

	vector<int> ret;
	const map< string, LuaValue >& t = v.toTable();
	for ( std::pair< string, LuaValue > a : t ) {
		int i = a.second.toInteger();
		dbg << ", " << i;
		ret.emplace_back( i );
	}

	dbg << " => Ok";
	return ret;
}


int Config::ArrayLength( const string& name )
{
	Debug dbg(Debug::Verbose);
	dbg + _debug_date() + self_thread() + __CLASS_NAME__ + "::" + __FUNCTION_NAME__ + "("; dbg.fDebug_top(name);

	LuaValue v = mLua->value( name );
	if ( v.type() != LuaValue::Table ) {
		dbg << " => not found !";
		return -1;
	}

	int ret = 0;
	const map< string, LuaValue >& t = v.toTable();
	for ( std::pair< string, LuaValue > a : t ) {
		ret++;
	}

	return ret;
}


int Config::LocateValue( const string& _name )
{
	return 0;
}


string Config::DumpVariable( const string& name, int index, int indent )
{
	return name + " = " + mLua->value( name ).serialize();
}


void Config::Execute( const string& code, bool silent )
{
	if ( not silent ) {
		fDebug( code );
	}
	mLua->do_string( code );
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


#define LUAdostring( s ) gDebug() << "dostring : " << string(s); mLua->do_string( string(s) );


void Config::Reload()
{
	gDebug() << "Reload 1 : " << _mem_usage();

	LUAdostring( "function Vector( x, y, z, w ) return { x = x or 0, y = y or 0, z = z or 0, w = w or 0 } end" );
	LUAdostring( "PID = setmetatable( {}, { __call = function ( p, i, d ) return { p = p or 0, i = i or 0, d = d or 0 } end } )" );

	LUAdostring( "board = { type = \"" + string( BOARD ) + "\" }" );
	LUAdostring( "system = { loop_time = 2000 }" );

	if ( mFilename != "" ) {
		int ret = mLua->do_file( mFilename );
		if ( ret != 0 ) {
			exit(0);
		}
	}
/*
	LUAdostring( "frame = { motors = {} }" );
	LUAdostring( "battery = {}" );
	LUAdostring( "camera = { v1 = {}, v2 = {}, hq = {} }" );
	LUAdostring( "hud = {}" );
	LUAdostring( "microphone = {}" );
	LUAdostring( "controller = {}" );
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
	LUAdostring( "function SPI( address, speed ) return { type = 'SPI', address = address, speed = speed } end" );
	gDebug() << "Reload 2 : " << _mem_usage();

#ifdef BUILD_motors
	for ( auto motor : Motor::knownMotors() ) {
		LUAdostring( "function " + motor.first + "( params ) params.motor_type = \"" + motor.first + "\" ; return params end" );
	}
#endif
	gDebug() << "Reload 3 : " << _mem_usage();
#ifdef BUILD_links
	for ( auto link : Link::knownLinks() ) {
		LUAdostring( "function " + link.first + "( params ) params.link_type = \"" + link.first + "\" ; return params end" );
	}
#endif
	gDebug() << "Reload 4 : " << _mem_usage();
#ifdef BUILD_sensors
	for ( const Sensor::Device& device : Sensor::KnownDevices() ) {
		LUAdostring( "function " + string(device.name) + "( params ) params.sensor_type = \"" + string(device.name) + "\" ; return params end" );
	}
#endif
	gDebug() << "Reload 5 : " << _mem_usage();


	if ( mFilename != "" ) {
		luaL_loadfile( L, mFilename.c_str() );
		int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );
		if ( ret != 0 ) {
			lua_Debug ar;
			lua_getstack(L, 1, &ar);
			lua_getinfo(L, "nSl", &ar);
			gDebug() << "Lua : Error while executing file \"" << mFilename << "\" : " << ar.currentline << " : \"" << lua_tostring( L, -1 ) << "\"";
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
			gDebug() << "Lua : Error while executing file \"" << mSettingsFilename << "\" : " << ar.currentline << " : \"" << lua_tostring( L, -1 ) << "\"";
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
				gDebug() << "mSettings[\"" << key << "\"] = '" << mSettings[ key ] << "'";
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
*/
}


void Config::Apply()
{
/*
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
		gDebug() << "On sensor \"" << it << "\"";
		Gyroscope* gyro = Sensor::gyroscope( it );
		if ( not gyro ) {
			Sensor::RegisterDevice( it, this, "gyroscopes[" + it + "]" );
			gyro = Sensor::gyroscope( it );
		}
		if ( gyro ) {
			gDebug() << "Gyroscope \"" << it << "\" found";
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = Integer( "gyroscopes." + it + ".axis_swap.x" );
			swap[1] = Integer( "gyroscopes." + it + ".axis_swap.y" );
			swap[2] = Integer( "gyroscopes." + it + ".axis_swap.z" );
			gyro->setAxisSwap( swap );
		}
	}
	for ( auto it : accelerometers ) {
		gDebug() << "On sensor \"" << it << "\"";
		Accelerometer* accel = Sensor::accelerometer( it );
		if ( accel ) {
			gDebug() << "Accelerometer \"" << it << "\" found";
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = Integer( "accelerometers." + it + ".axis_swap.x" );
			swap[1] = Integer( "accelerometers." + it + ".axis_swap.y" );
			swap[2] = Integer( "accelerometers." + it + ".axis_swap.z" );
			accel->setAxisSwap( swap );
		}
	}
	for ( auto it : magnetometers ) {
		gDebug() << "On sensor \"" << it << "\"";
		Magnetometer* magn = Sensor::magnetometer( it );
		if ( magn ) {
			gDebug() << "Magnetometers \"" << it << "\" found";
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = Integer( "magnetometers." + it + ".axis_swap.x" );
			swap[1] = Integer( "magnetometers." + it + ".axis_swap.y" );
			swap[2] = Integer( "magnetometers." + it + ".axis_swap.z" );
			magn->setAxisSwap( swap );
		}
	}
#endif // BUILD_sensors
*/
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
