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
#include <Main.h>
#include "LSM303.h"

int LSM303::flight_register( Main* main )
{
	Device dev1;
	Device dev2;

	dev1.iI2CAddr = 0x19;
	dev1.name = "LSM303Accel";
	dev1.fInstanciate = LSM303Accel::Instanciate;

	dev2.iI2CAddr = 0x1E;
	dev2.name = "LSM303Mag";
	dev2.fInstanciate = LSM303Mag::Instanciate;

	mKnownDevices.push_back( dev1 );
	mKnownDevices.push_back( dev2 );
	return 0;
}


Sensor* LSM303Mag::Instanciate( Config* config, const string& object, Bus* bus )
{
	// TODO : use bus
	return new LSM303Mag();
}


Sensor* LSM303Accel::Instanciate( Config* config, const string& object, Bus* bus )
{
	// TODO : use bus
	return new LSM303Accel();
}


LSM303Accel::LSM303Accel()
	: Accelerometer()
	, mI2C( new I2C( 0x19 ) )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { "LSM303", "lsm303", "lsm303dlhc" };
	mAxes[0] = true;
	mAxes[1] = true;
	mAxes[2] = true;

	mI2C->Write8( LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0b01110111 );
// 	mI2C->Write8( LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0b01000111 );
// 	mI2C->Write8( LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0b10010111 );
	mI2C->Write8( LSM303_REGISTER_ACCEL_CTRL_REG4_A, 0b00101000 );
	mI2C->Write8( LSM303_REGISTER_ACCEL_CTRL_REG5_A, 0b01000000 );
}


LSM303Mag::LSM303Mag()
	: Magnetometer()
	, mI2C( new I2C( 0x1E ) )
{
	mNames = { "LSM303", "lsm303", "lsm303dlhc" };
	mAxes[0] = true;
	mAxes[1] = true;
	mAxes[2] = true;

	mI2C->Write8( LSM303_REGISTER_MAG_MR_REG_M, 0x00 );
}


LSM303Accel::~LSM303Accel()
{
}


LSM303Mag::~LSM303Mag()
{
}


void LSM303Accel::Calibrate( float dt, bool last_pass )
{
	Vector3f accel;
	Read( &accel, true );
	mCalibrationAccum += Vector4f( accel, 1.0f );

	if ( last_pass ) {
		mOffset = mCalibrationAccum.xyz() / mCalibrationAccum.w;
		mCalibrationAccum = Vector4f();
		mCalibrated = true;
		gDebug() << "LSM303 SAVING CALIBRATED OFFSETS !\n";
		aDebug( "Offset", mOffset.x, mOffset.y, mOffset.z );
		Board::SaveRegister( "LSM303:Accelerometer:Offset:X", to_string( mOffset.x ) );
		Board::SaveRegister( "LSM303:Accelerometer:Offset:Y", to_string( mOffset.y ) );
		Board::SaveRegister( "LSM303:Accelerometer:Offset:Z", to_string( mOffset.z ) );
	}
}


void LSM303Mag::Calibrate( float dt, bool last_pass )
{
	// TODO
}


void LSM303Accel::Read( Vector3f* v, bool raw )
{
	short saccel[3] = { 0 };
	uint8_t status = 0;

	do {
		mI2C->Read8( LSM303_REGISTER_ACCEL_STATUS_REG_A, &status );
		usleep( 0 );
	} while ( !( status & 0b00001111 ) );

	mI2C->Read( LSM303_REGISTER_ACCEL_Oraw_temp_X_L_A | 0x80, saccel, sizeof(saccel) );
	v->x = ACCEL_MS2( saccel[0] );
	v->y = ACCEL_MS2( saccel[1] );
	v->z = ACCEL_MS2( saccel[2] );

	v->x -= mOffset.x;
	v->y -= mOffset.y;
	ApplySwap( *v );

	mLastValues = *v;
}


void LSM303Mag::Read( Vector3f* v, bool raw )
{
	uint16_t _smag[3] = { 0 };
	int16_t smag[3] = { 0 };

	mI2C->Read( LSM303_REGISTER_MAG_Oraw_temp_X_H_M, _smag, sizeof(_smag) );

	smag[0] = ( ( ( _smag[0] >> 8 ) & 0xFF ) | ( ( _smag[0] << 8 ) & 0x0F00 ) ) << 4;
	smag[1] = ( ( ( _smag[1] >> 8 ) & 0xFF ) | ( ( _smag[1] << 8 ) & 0x0F00 ) ) << 4;
	smag[2] = ( ( ( _smag[2] >> 8 ) & 0xFF ) | ( ( _smag[2] << 8 ) & 0x0F00 ) ) << 4;

	v->x = (float)smag[0] / 1.0f;
	v->y = (float)smag[1] / 1.0f;
	v->z = (float)smag[2] / 1.0f;

	ApplySwap( *v );

	mLastValues = *v;
}
