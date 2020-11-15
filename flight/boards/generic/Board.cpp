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

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fstream>
#include <Main.h>
#include <netinet/in.h>
#include "Board.h"
#include "I2C.h"


uint64_t Board::mTicksBase = 0;
decltype(Board::mRegisters) Board::mRegisters = decltype(Board::mRegisters)();
map< string, bool > Board::mDefectivePeripherals;


Board::Board( Main* main )
{
	srand( time( nullptr ) );
	// hardware-specific initialization should be done here or directly in startup.cpp
	// this->mRegisters should be loaded here
}


Board::~Board()
{
}


static string readproc( const string& filename, const string& entry = "", const string& delim = ":" )
{
	char buf[1024] = "";
	string res = "";
	ifstream file( filename );

	if ( entry.length() == 0 or entry == "" ) {
		file.readsome( buf, sizeof( buf ) );
		res = buf;
	} else {
		while ( file.good() ) {
			file.getline( buf, sizeof(buf), '\n' );
			if ( strstr( buf, entry.c_str() ) ) {
				char* s = strstr( buf, delim.c_str() ) + delim.length();
				while ( *s == ' ' or *s == '\t' ) {
					s++;
				}
				char* end = s;
				while ( *end != '\n' and *end++ );
				*end = 0;
				res = string( s );
				break;
			}
		}
	}

	res = res.substr( 0, res.find( "\n" ) );
	file.close();
	return res;
}


static string readcmd( const string& cmd, const string& entry = "", const string& delim = ":" )
{
	char buf[1024] = "";
	string res = "";
	FILE* fp = popen( cmd.c_str(), "rb" );

	if ( entry.length() == 0 or entry == "" ) {
		fread( buf, 1, sizeof( buf ), fp );
		res = buf;
	} else {
		// TODO
	}

	pclose( fp );
	return res;
}


map< string, bool >& Board::defectivePeripherals()
{
	return mDefectivePeripherals;
}


string Board::infos()
{
	string res = "";

	res += "Type:" BOARD "\n";
	res += "Firmware version:" VERSION_STRING "\n";
	res += "CPU:" + readproc( "/proc/cpuinfo", "model name" ) + string( "\n" );
	res += "CPU frequency:" + to_string( atoi( readproc( "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq" ).c_str() ) / 1000 ) + string( "MHz\n" );
	res += "CPU cores count:" + to_string( sysconf( _SC_NPROCESSORS_ONLN ) ) + string( "\n" );

	return res;
}


void Board::InformLoading( int force_led )
{
	// Controls loading LED.
	// if force_led != -1, then this function should turn on (1) or off (0) the led according to this variablezz
	// Otherwise, make it blink
}


void Board::LoadingDone()
{
	InformLoading( true );
}


void Board::setLocalTimestamp( uint32_t t )
{
}


Board::Date Board::localDate()
{
	Date ret;
	memset( &ret, 0, sizeof(ret) );
	return ret;
}



const uint32_t Board::LoadRegisterU32( const string& name, uint32_t def )
{
	string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return strtoul( str.c_str(), nullptr, 10 );
	}
	return def;
}


const float Board::LoadRegisterFloat( const string& name, float def )
{
	string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return atof( str.c_str() );
	}
	return def;
}


const string Board::LoadRegister( const string& name )
{
	if ( mRegisters.find( name ) != mRegisters.end() ) {
		return mRegisters[ name ];
	}
	return "";
}


int Board::SaveRegister( const string& name, const string& value )
{
	mRegisters[ name ] = value;

	// Save register(s) here, and return 0 on success or -1 on error

	return 0;
}


uint64_t Board::GetTicks()
{
	if ( mTicksBase == 0 ) {
		struct timespec now;
		clock_gettime( CLOCK_MONOTONIC, &now );
		mTicksBase = (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
	}

	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL - mTicksBase;
}


uint64_t Board::WaitTick( uint64_t ticks_p_second, uint64_t lastTick, uint64_t sleep_bias )
{
	uint64_t ticks = GetTicks();

	if ( ( ticks - lastTick ) < ticks_p_second ) {
		int64_t t = (int64_t)ticks_p_second - ( (int64_t)ticks - (int64_t)lastTick ) + sleep_bias;
		if ( t < 0 ) {
			t = 0;
		}
		usleep( t );
	}

	return GetTicks();
}


string Board::readcmd( const string& cmd, const string& entry, const string& delim )
{
	char buf[1024] = "";
	string res = "";
	FILE* fp = popen( cmd.c_str(), "r" );
	if ( !fp ) {
		printf( "popen failed : %s\n", strerror( errno ) );
		return "";
	}

	if ( entry.length() == 0 or entry == "" ) {
		fread( buf, 1, sizeof( buf ), fp );
		res = buf;
	} else {
		while ( fgets( buf, sizeof(buf), fp ) ) {
			if ( strstr( buf, entry.c_str() ) ) {
				char* s = strstr( buf, delim.c_str() ) + delim.length();
				while ( *s == ' ' or *s == '\t' ) {
					s++;
				}
				char* end = s;
				while ( *end != '\n' and *end++ );
				*end = 0;
				res = string( s );
				break;
			}
		}
	}

	pclose( fp );
	return res;
}


uint32_t Board::CPULoad()
{
	return 0;
}


uint32_t Board::CPUTemp()
{
	return 0;
}


void Board::EnableTunDevice()
{
}


void Board::DisableTunDevice()
{
}


void Board::UpdateFirmwareData( const uint8_t* buf, uint32_t offset, uint32_t size )
{
}


void Board::UpdateFirmwareProcess( uint32_t crc )
{
}


void Board::Reset()
{
}


extern "C" uint32_t _mem_usage()
{
	return 0; // TODO
}


uint16_t board_htons( uint16_t v )
{
	return htons( v );
}


uint16_t board_ntohs( uint16_t v )
{
	return ntohs( v );
}


uint32_t board_htonl( uint32_t v )
{
	return htonl( v );
}


uint32_t board_ntohl( uint32_t v )
{
	return ntohl( v );
}
