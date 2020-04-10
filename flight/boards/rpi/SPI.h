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
#include <mutex>
#include <string>
#include <linux/spi/spidev.h>
#include "../common/Bus.h"

class SPI : public Bus
{
public:
	SPI( const string& device, uint32_t speed_hz = 500000 ); // setting speed to 0 means slave-mode
	~SPI();
	const string& device() const;

	int Transfer( void* tx, void* rx, uint32_t len );
	int Read( void* buf, uint32_t len );
	int Write( const void* buf, uint32_t len );
	int Read( uint8_t reg, void* buf, uint32_t len );
	int Write( uint8_t reg, const void* buf, uint32_t len );


private:
	string mDevice;
	int mFD;
	int mBitsPerWord;
	struct spi_ioc_transfer mXFer[10];
	mutex mTransferMutex;
};

#endif
