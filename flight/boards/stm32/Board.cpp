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

#include BOARD_VARIANT_FILE
#include "Board.h"


extern "C" uint32_t board_clock_speed();
extern "C" void board_ticks_reset();
extern "C" uint32_t board_ticks();
extern "C" uint32_t _mem_usage();
extern "C" uint32_t _mem_size();

map< string, bool > Board::mDefectivePeripherals;

Board::Board( Main* main )
{
}


Board::~Board()
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


void Board::InformLoading( int force_led )
{
	static uint64_t ticks = 0;
	static bool led_state = true;
	char cmd[256] = "";

	if ( force_led == 0 or force_led == 1 or GetTicks() - ticks >= 1000 * 250 ) {
		ticks = GetTicks();
		led_state = !led_state;
		if ( force_led == 0 or force_led == 1 ) {
			led_state = force_led;
		}
// 		sprintf( cmd, "echo %d > /sys/class/leds/led0/brightness", led_state );
// 		system( cmd );
	}
}


void Board::LoadingDone()
{
	InformLoading( 1 );
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
	res += "Model:" BOARD_VARIANT "\n";
#ifdef __CORTEX_M
	res += "CPU:Cortex M" + to_string(__CORTEX_M) + "\n";
#endif
	res += "CPU cores count:1\n";
	res += "CPU frequency:" + to_string( board_clock_speed() / 1000000 ) + "MHz\n";
// 	res += "SoC voltage:" + readcmd( "vcgencmd measure_volts core", "volt", "=" ) + string( "\n" );
// 	res += "RAM:" + readcmd( "vcgencmd get_mem arm", "arm", "=" ) + string( "\n" );

	return res;
}


const uint32_t Board::LoadRegisterU32( const string& name, uint32_t def )
{
/*
	string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return strtoul( str.c_str(), nullptr, 10 );
	}
*/
	return def;
}


const float Board::LoadRegisterFloat( const string& name, float def )
{
/*
	string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return atof( str.c_str() );
	}
*/
	return def;
}


const string Board::LoadRegister( const string& name )
{
/*
	if ( mRegisters.find( name ) != mRegisters.end() ) {
		return mRegisters[ name ];
	}
*/
	return "";
}


int Board::SaveRegister( const string& name, const string& value )
{
/*
	mRegisters[ name ] = value;
	// TODO
*/
	return -1;
}


uint64_t Board::GetTicks()
{
	return board_ticks();
}


uint32_t Board::CPULoad()
{
	return 0;
}


uint32_t Board::CPUTemp()
{
	return 0;
}


uint32_t Board::FreeDiskSpace()
{
	return 0;
}


void Board::setDiskFull()
{
}
