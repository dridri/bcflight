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
#include "L3GD20H.h"

int L3GD20H::flight_register( Main* main )
{
	Device dev;
	dev.iI2CAddr = 0x6b;
	dev.name = "L3GD20H";
	dev.fInstanciate = L3GD20H::Instanciate;
	mKnownDevices.push_back( dev );
	return 0;
}


Gyroscope* L3GD20H::Instanciate( Config* config, const std::string& object )
{
	return new L3GD20H();
}


L3GD20H::L3GD20H()
	: Gyroscope()
	, mI2C( new I2C( 0x6b ) )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { "L3GD20", "l3gd20", "l3gd20h" };
	mAxes[0] = true;
	mAxes[1] = true;
	mAxes[2] = true;

	mI2C->Write8( L3GD20_CTRL_REG2, 0b00000000 );
	mI2C->Write8( L3GD20_CTRL_REG3, 0b00001000 );
	mI2C->Write8( L3GD20_CTRL_REG4, 0b00110000 );
	mI2C->Write8( L3GD20_CTRL_REG5, 0b01000000 );
	mI2C->Write8( L3GD20_FIFO_CTRL_REG, 0b01000000 );
	mI2C->Write8( L3GD20_CTRL_REG1, 0b11111111 );
}


L3GD20H::~L3GD20H()
{
}


void L3GD20H::Calibrate( float dt, bool last_pass )
{
	Vector3f gyro;
	Read( &gyro, true );
	mCalibrationAccum += Vector4f( gyro, 1.0f );

	if ( last_pass ) {
		mOffset = mCalibrationAccum.xyz() / mCalibrationAccum.w;
		aDebug( "Offset", mOffset.x, mOffset.y, mOffset.z );
		mCalibrationAccum = Vector4f();
		mCalibrated = true;
	}
}


void L3GD20H::Read( Vector3f* v, bool raw )
{
	short sgyro[3] = { 0 };

	mI2C->Read( L3GD20_OUT_X_L | 0x80, sgyro, sizeof(sgyro) );
	v->x = 0.0703125f * (float)sgyro[0];
	v->y = 0.0703125f * (float)sgyro[1];
	v->z = 0.0703125f * (float)sgyro[2];

	v->operator-=( mOffset );
	ApplySwap( *v );

	mLastValues = *v;
}
