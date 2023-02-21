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
#include "ADS1015.h"


#define ADS1015_REG_POINTER_MASK        (0x03)
#define ADS1015_REG_POINTER_CONVERT     (0x00)
#define ADS1015_REG_POINTER_CONFIG      (0x01)
#define ADS1015_REG_POINTER_LOWTHRESH   (0x02)
#define ADS1015_REG_POINTER_HITHRESH    (0x03)

#define ADS1015_REG_CONFIG_OS_MASK      (0x8000)
#define ADS1015_REG_CONFIG_OS_SINGLE    (0x8000)  // Write: Set to start a single-conversion
#define ADS1015_REG_CONFIG_OS_BUSY      (0x0000)  // Read: Bit = 0 when conversion is in progress
#define ADS1015_REG_CONFIG_OS_NOTBUSY   (0x8000)  // Read: Bit = 1 when device is not performing a conversion

#define ADS1015_REG_CONFIG_MUX_MASK     (0x7000)
#define ADS1015_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
#define ADS1015_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
#define ADS1015_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
#define ADS1015_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
#define ADS1015_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
#define ADS1015_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
#define ADS1015_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
#define ADS1015_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3

#define ADS1015_REG_CONFIG_PGA_MASK     (0x0E00)
#define ADS1015_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
#define ADS1015_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
#define ADS1015_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
#define ADS1015_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
#define ADS1015_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
#define ADS1015_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16

#define ADS1015_REG_CONFIG_MODE_MASK    (0x0100)
#define ADS1015_REG_CONFIG_MODE_CONTIN  (0x0000)  // Continuous conversion mode
#define ADS1015_REG_CONFIG_MODE_SINGLE  (0x0100)  // Power-down single-shot mode (default)

#define ADS1015_REG_CONFIG_DR_MASK      (0x00E0)  
#define ADS1015_REG_CONFIG_DR_128SPS    (0x0000)  // 128 samples per second
#define ADS1015_REG_CONFIG_DR_250SPS    (0x0020)  // 250 samples per second
#define ADS1015_REG_CONFIG_DR_490SPS    (0x0040)  // 490 samples per second
#define ADS1015_REG_CONFIG_DR_920SPS    (0x0060)  // 920 samples per second
#define ADS1015_REG_CONFIG_DR_1600SPS   (0x0080)  // 1600 samples per second (default)
#define ADS1015_REG_CONFIG_DR_2400SPS   (0x00A0)  // 2400 samples per second
#define ADS1015_REG_CONFIG_DR_3300SPS   (0x00C0)  // 3300 samples per second

#define ADS1015_REG_CONFIG_CMODE_MASK   (0x0010)
#define ADS1015_REG_CONFIG_CMODE_TRAD   (0x0000)  // Traditional comparator with hysteresis (default)
#define ADS1015_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

#define ADS1015_REG_CONFIG_CPOL_MASK    (0x0008)
#define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
#define ADS1015_REG_CONFIG_CPOL_ACTVHI  (0x0008)  // ALERT/RDY pin is high when active

#define ADS1015_REG_CONFIG_CLAT_MASK    (0x0004)  // Determines if ALERT/RDY pin latches once asserted
#define ADS1015_REG_CONFIG_CLAT_NONLAT  (0x0000)  // Non-latching comparator (default)
#define ADS1015_REG_CONFIG_CLAT_LATCH   (0x0004)  // Latching comparator

#define ADS1015_REG_CONFIG_CQUE_MASK    (0x0003)
#define ADS1015_REG_CONFIG_CQUE_1CONV   (0x0000)  // Assert ALERT/RDY after one conversions
#define ADS1015_REG_CONFIG_CQUE_2CONV   (0x0001)  // Assert ALERT/RDY after two conversions
#define ADS1015_REG_CONFIG_CQUE_4CONV   (0x0002)  // Assert ALERT/RDY after four conversions
#define ADS1015_REG_CONFIG_CQUE_NONE    (0x0003)  // Disable the comparator and put ALERT/RDY in high state (default)


ADS1015::ADS1015()
	: mI2C( new I2C( 0x48 ) )
	, mRingBuffer{ 0.0f }
	, mRingSum( 0.0f )
	, mRingIndex( 0 )
	, mChannelReady{ false }
{
	mNames = { "ADS1015" };
	mI2C->Connect();
	usleep( 1000 * 100 );
}


ADS1015::~ADS1015()
{
	delete mI2C;
}


void ADS1015::Calibrate( float dt, bool last_pass )
{
	(void)dt;
}


float ADS1015::Read( int channel )
{
	uint16_t _ret1 = 0;
	uint16_t _ret2 = 0;
	int r1 = 0;
	int r2 = 0;
	int r3 = 0;

	// if ( not mChannelReady[channel] ) {
		uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
						ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
						ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
						ADS1015_REG_CONFIG_CMODE_WINDOW   | // Traditional comparator (default val)
						ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
						ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

		config |= ADS1015_REG_CONFIG_PGA_4_096V;
		config |= ( ADS1015_REG_CONFIG_MUX_SINGLE_0 + ( channel << 12 ) );
		config |= ADS1015_REG_CONFIG_OS_SINGLE;
		// config |= ADS1015_REG_CONFIG_OS_NOTBUSY;
		config = ( ( config << 8 ) & 0xFF00 ) | ( ( config >> 8 ) & 0xFF );

		r1 = mI2C->Write16( ADS1015_REG_POINTER_CONFIG, config );
		usleep( 10000 );
		// mChannelReady[channel] = true;
	// }

	r2 = mI2C->Read16( ADS1015_REG_POINTER_CONVERT, &_ret1 );
	r3 = mI2C->Read16( ADS1015_REG_POINTER_CONVERT, &_ret2 );
	_ret1 = ( ( _ret1 << 8 ) & 0xFF00 ) | ( ( _ret1 >> 8 ) & 0xFF );
	_ret2 = ( ( _ret2 << 8 ) & 0xFF00 ) | ( ( _ret2 >> 8 ) & 0xFF );
	// printf("r1, r2, r3 : %d, %d, %d | %d, %d\n", r1, r2, r3, _ret1, _ret2);

	if ( r1 < 0 or r2 < 0 or r3 < 0 or std::abs((int)_ret1 - (int)_ret2) > 128 ) {
		// printf("'%s'\n", strerror(errno));
		throw std::exception();
	}

	// float fret = (float)( ret * 6.144f / 32768.0f );
	float fret = (float)( _ret1 * 4.096f / 32768.0f );

	return fret;
}


string ADS1015::infos()
{
	return "I2Caddr = " + to_string( mI2C->address() ) + ", " + "Resolution = \"12 bits\", " + "Channels = 4";
}
