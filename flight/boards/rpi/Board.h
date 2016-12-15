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

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <list>
#include <string>
#include <map>

extern "C" {
#include <interface/vchi/vchi.h>
};

class Main;

class Board
{
public:
	typedef struct Date {
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
	} Date;

	Board( Main* main );
	~Board();

	static std::string infos();

	static void InformLoading( int force_led = -1 );
	static void LoadingDone();

	static void setLocalTimestamp( uint32_t t );
	static Date localDate();

	static const std::string LoadRegister( const std::string& name );
	static const uint32_t LoadRegisterU32( const std::string& name, uint32_t def = 0 );
	static const float LoadRegisterFloat( const std::string& name, float def = 0.0f );
	static int SaveRegister( const std::string& name, const std::string& value );

	static uint64_t GetTicks();
	static uint64_t WaitTick( uint64_t ticks_p_second, uint64_t lastTick, uint64_t sleep_bias = -500 );

	static std::string readcmd( const std::string& cmd, const std::string& entry = "", const std::string& delim = ":" );
	static uint32_t CPULoad();
	static uint32_t CPUTemp();

	static void EnableTunDevice();
	static void DisableTunDevice();
	static void UpdateFirmwareData( const uint8_t* buf, uint32_t offset, uint32_t size );
	static void UpdateFirmwareProcess( uint32_t crc );
	static void Reset();

private:
	static uint64_t mTicksBase;
	static std::map< std::string, std::string > mRegisters;
	static uint64_t mLastWorkJiffies;
	static uint64_t mLastTotalJiffies;
	static bool mUpdating;

	static void AtExit();

	static void VCOSInit();
	static VCHI_INSTANCE_T global_initialise_instance;
	static VCHI_CONNECTION_T* global_connection;
};

#endif // BOARD_H
