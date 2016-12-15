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
#include "Board.h"
#include "I2C.h"


uint64_t Board::mTicksBase = 0;
decltype(Board::mRegisters) Board::mRegisters = decltype(Board::mRegisters)();


Board::Board( Main* main )
{
	srand( time( nullptr ) );
	// hardware-specific initialization should be done here or directly in startup.cpp
	// this->mRegisters should be loaded here
}


Board::~Board()
{
}


static std::string readproc( const std::string& filename, const std::string& entry = "", const std::string& delim = ":" )
{
	char buf[1024] = "";
	std::string res = "";
	std::ifstream file( filename );

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
				res = std::string( s );
				break;
			}
		}
	}

	res = res.substr( 0, res.find( "\n" ) );
	file.close();
	return res;
}


static std::string readcmd( const std::string& cmd, const std::string& entry = "", const std::string& delim = ":" )
{
	char buf[1024] = "";
	std::string res = "";
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


std::string Board::infos()
{
	std::string res = "";

	res += "Type:" BOARD "\n";
	res += "Firmware version:" VERSION_STRING "\n";
	res += "CPU:" + readproc( "/proc/cpuinfo", "model name" ) + std::string( "\n" );
	res += "CPU frequency:" + std::to_string( std::atoi( readproc( "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq" ).c_str() ) / 1000 ) + std::string( "MHz\n" );
	res += "CPU cores count:" + std::to_string( sysconf( _SC_NPROCESSORS_ONLN ) ) + std::string( "\n" );

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



const uint32_t Board::LoadRegisterU32( const std::string& name, uint32_t def )
{
	std::string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return std::strtoul( str.c_str(), nullptr, 10 );
	}
	return def;
}


const float Board::LoadRegisterFloat( const std::string& name, float def )
{
	std::string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return std::atof( str.c_str() );
	}
	return def;
}


const std::string Board::LoadRegister( const std::string& name )
{
	if ( mRegisters.find( name ) != mRegisters.end() ) {
		return mRegisters[ name ];
	}
	return "";
}


int Board::SaveRegister( const std::string& name, const std::string& value )
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


std::string Board::readcmd( const std::string& cmd, const std::string& entry, const std::string& delim )
{
	char buf[1024] = "";
	std::string res = "";
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
				res = std::string( s );
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
