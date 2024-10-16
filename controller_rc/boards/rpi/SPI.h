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

#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <list>
#include <string>
#include <linux/spi/spidev.h>
#include <Lua.h>

using namespace std;


class SPI
{
public:
	LUA_EXPORT SPI( const string& device, uint32_t speed_hz = 500000 );
	~SPI();
	int Connect();
	const string& device() const;

	int Transfer( void* tx, void* rx, uint32_t len );

	LuaValue infos();

private:
	LUA_PROPERTY("device") string mDevice;
	uint32_t mSpeedHz;
	int mFD;
	int mBitsPerWord;
	struct spi_ioc_transfer mXFer[10];
	mutex mTransferMutex;
};

#endif
