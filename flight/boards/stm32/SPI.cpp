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

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "Debug.h"
#include "SPI.h"
#include "GPIO.h"


SPI::SPI( const string& device, uint32_t speed_hz )
	: Bus()
	, mDevice( device )
{
	fDebug( device, speed_hz );
	bool slave = ( speed_hz == 0 );

	if ( device[3] == '1' ) {
		__HAL_RCC_SPI1_CLK_ENABLE();
		mHandle.Instance = SPI1;
	} else if ( device[3] == '2' ) {
		__HAL_RCC_SPI2_CLK_ENABLE();
		mHandle.Instance = SPI2;
	} else if ( device[3] == '3' ) {
		__HAL_RCC_SPI3_CLK_ENABLE();
		mHandle.Instance = SPI3;
#ifdef SPI4
	} else if ( device[3] == '4' ) {
		__HAL_RCC_SPI4_CLK_ENABLE();
		mHandle.Instance = SPI4;
#endif
#ifdef SPI5
	} else if ( device[3] == '5' ) {
		__HAL_RCC_SPI5_CLK_ENABLE();
		mHandle.Instance = SPI5;
#endif
	}
	mHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	mHandle.Init.Direction = SPI_DIRECTION_2LINES;
	mHandle.Init.CLKPhase = SPI_PHASE_2EDGE;
	mHandle.Init.CLKPolarity = SPI_POLARITY_HIGH;
	mHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	mHandle.Init.DataSize = SPI_DATASIZE_8BIT;
	mHandle.Init.FirstBit = SPI_FIRSTBIT_LSB;
	mHandle.Init.NSS = ( slave ? ( SPI_NSS_HARD_INPUT ) : ( SPI_NSS_HARD_OUTPUT ) );
	mHandle.Init.TIMode = SPI_TIMODE_DISABLED;
	mHandle.Init.Mode = ( slave ? ( SPI_MODE_SLAVE ) : ( SPI_MODE_MASTER ) );
	if ( HAL_SPI_Init( &mHandle ) != HAL_OK ) {
	}

#ifdef VARIANT_GROUP_stm32f4
	if ( device[3] == '1' ) {
		if ( slave ) GPIO::setMode( 0 << 4 | 4, GPIO::OpenDrain, GPIO::PullUp, GPIO_AF5_SPI1 ); // Chip-Select
		GPIO::setMode( 0 << 4 | 5, slave ? GPIO::OpenDrain : GPIO::Output, GPIO::PullUp, GPIO_AF5_SPI1 ); // SCK
		GPIO::setMode( 0 << 4 | 6, slave ? GPIO::Output : GPIO::OpenDrain, GPIO::PullUp, GPIO_AF5_SPI1 ); // MISO
		GPIO::setMode( 0 << 4 | 7, slave ? GPIO::OpenDrain : GPIO::Output, GPIO::PullUp, GPIO_AF5_SPI1 ); // MOSI
	} else if ( device[3] == '2' ) {
		if ( slave ) GPIO::setMode( 1 << 4 | 12, GPIO::OpenDrain, GPIO::PullUp, GPIO_AF5_SPI2 ); // Chip-Select
		GPIO::setMode( 1 << 4 | 13, slave ? GPIO::OpenDrain : GPIO::Output, GPIO::PullUp, GPIO_AF5_SPI2 ); // SCK
		GPIO::setMode( 1 << 4 | 14, slave ? GPIO::Output : GPIO::OpenDrain, GPIO::PullUp, GPIO_AF5_SPI2 ); // MISO
		GPIO::setMode( 1 << 4 | 15, slave ? GPIO::OpenDrain : GPIO::Output, GPIO::PullUp, GPIO_AF5_SPI2 ); // MOSI
	} else if ( device[3] == '3' ) {
#ifdef SPI4
	} else if ( device[3] == '4' ) {
#endif
#ifdef SPI5
	} else if ( device[3] == '5' ) {
#endif
	}
#endif
}


SPI::~SPI()
{
	HAL_SPI_DeInit( &mHandle );
}


const string& SPI::device() const
{
	return mDevice;
}


int SPI::Read( void* buf, uint32_t len )
{
	auto ret = HAL_SPI_Receive( &mHandle, (uint8_t*)buf, len, HAL_MAX_DELAY );
	return ( ret == HAL_OK ? len : -1 );
}


int SPI::Write( const void* buf, uint32_t len )
{
	auto ret = HAL_SPI_Transmit( &mHandle, (uint8_t*)buf, len, HAL_MAX_DELAY );
	return ( ret == HAL_OK ? len : -1 );
}


int SPI::Read( uint8_t reg, void* buf, uint32_t len )
{
	auto ret = HAL_SPI_Transmit( &mHandle, &reg, sizeof(reg), HAL_MAX_DELAY );
	if ( ret != HAL_OK ) {
		return -1;
	}
	ret = HAL_SPI_Receive( &mHandle, (uint8_t*)buf, len, HAL_MAX_DELAY );
	return ( ret == HAL_OK ? len : -1 );
}


int SPI::Write( uint8_t reg, const void* buf, uint32_t len )
{
	auto ret = HAL_SPI_Transmit( &mHandle, &reg, sizeof(reg), HAL_MAX_DELAY );
	if ( ret != HAL_OK ) {
		return -1;
	}
	ret = HAL_SPI_Transmit( &mHandle, (uint8_t*)buf, len, HAL_MAX_DELAY );
	return ( ret == HAL_OK ? len : -1 );
}


int SPI::Transfer( const void* tx, void* rx, uint32_t len )
{
	auto ret = HAL_SPI_TransmitReceive( &mHandle, (uint8_t*)tx, (uint8_t*)rx, len, HAL_MAX_DELAY );
	return ( ret == HAL_OK ? len : -1 );
}
