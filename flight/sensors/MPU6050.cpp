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

#include <cmath>
#include <unistd.h>
#include "MPU6050.h"

int MPU6050::flight_register( Main* main )
{
	Device dev1;
	Device dev2;

	dev1.iI2CAddr = 0x68;
	dev1.name = "MPU6050";
	dev1.fInstanciate = []( Config* config, const std::string& object ) { return MPU6050::Instanciate( 0x68, config, object ); };

	dev2.iI2CAddr = 0x69;
	dev2.name = "MPU6050";
	dev2.fInstanciate = []( Config* config, const std::string& object ) { return MPU6050::Instanciate( 0x69, config, object ); };

	mKnownDevices.push_back( dev1 );
	mKnownDevices.push_back( dev2 );
	return 0;
}


Sensor* MPU6050::Instanciate( uint8_t i2c_addr, Config* config, const std::string& object )
{
	I2C* i2c = new I2C( i2c_addr );

	uint8_t whoami = 0x00;
	i2c->Read8( MPU_6050_WHO_AM_I, &whoami );
	bool mpu9250 = ( whoami == 0x71 or whoami == 0x73 );
	bool icm20608 = ( whoami == 0xAF );

	gDebug() << "whoami [0x" << std::hex << (int)i2c_addr << "]0x" << std::hex << (int)whoami << "\n";

	if ( mpu9250 ) {
		gDebug() << "I2C module at address 0x" << std::hex << (int)i2c->address() << " is MPU9250\n";
	} else if ( icm20608 ) {
		gDebug() << "I2C module at address 0x" << std::hex << (int)i2c->address() << " is ICM28608\n";
	}

	// No power management, internal clock source: 0b00000000
	i2c->Write8( MPU_6050_PWR_MGMT_1, 0b00000000 );
	// Disable all interrupts: 0b00000000
	i2c->Write8( MPU_6050_INT_ENABLE, 0b00000000 );
	// Disable all FIFOs: 0b00000000
	i2c->Write8( MPU_6050_FIFO_EN, 0b00000000 );
	// 1 kHz sampling rate: 0b00000000
	i2c->Write8( MPU_6050_SMPRT_DIV, 0b00000000 );
	// No ext sync, DLPF at 94Hz for the accel and 98Hz for the gyro: 0b00000010) (~200Hz: 0b00000001)
	i2c->Write8( MPU_6050_DEFINE, 0b00000011 );
	// Gyro range at +/-2000 °/s
	i2c->Write8( MPU_6050_GYRO_CONFIG, 0b00011000 );
	// Accel range at +/-16g
	i2c->Write8( MPU_6050_ACCEL_CONFIG, 0b00011000 );
	// Bypass mode enabled: 0b00000010
	i2c->Write8( MPU_6050_INT_PIN_CFG, 0b00000010 );
	// No FIFO and no I2C slaves: 0b00000000
	i2c->Write8( MPU_6050_USER_CTRL, 0b00000000 );

	if ( mpu9250 or icm20608 ) {
		// DLPF at 99Hz for the accel
		i2c->Write8( MPU_6050_ACCEL_CONFIG_2, 0b00000110 );
		I2C* i2c_mag = new I2C( MPU_6050_I2C_MAGN_ADDRESS );
		i2c_mag->Write8( MPU_6050_CNTL, 0x00 );
		delete i2c_mag;
	}

	// TODO : use config ('use_dmp')
	// TODO : see https://github.com/jrowberg/i2cdevlib/blob/master/Arduino/MPU6050/MPU6050_6Axis_MotionApps20.h
	if ( false ) {
		MPU6050* mpu = new MPU6050( i2c );
		delete mpu;
		exit(0);
	}
	delete i2c;

	// Manually add sensors to mDevices since they use the same address
	Sensor* mag = new MPU6050Mag( i2c_addr );
	Sensor* gyro = new MPU6050Gyro( i2c_addr );
	Sensor* accel = new MPU6050Accel( i2c_addr );
	mDevices.push_back( mag );
	mDevices.push_back( accel );

	return gyro;
}


MPU6050Accel::MPU6050Accel( int addr )
	: Accelerometer()
	, mI2C( new I2C( addr ) )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { "MPU6050" };

	mOffset.x = std::atof( Board::LoadRegister( "MPU6050:Accelerometer:Offset:X" ).c_str() );
	mOffset.y = std::atof( Board::LoadRegister( "MPU6050:Accelerometer:Offset:Y" ).c_str() );
	mOffset.z = std::atof( Board::LoadRegister( "MPU6050:Accelerometer:Offset:Z" ).c_str() );
	if ( mOffset.x != 0.0f and mOffset.y != 0.0f and mOffset.z != 0.0f ) {
		mCalibrated = true;
	}
}


MPU6050Gyro::MPU6050Gyro( int addr )
	: Gyroscope()
	, mI2C( new I2C( addr ) )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { "MPU6050" };
}


MPU6050Mag::MPU6050Mag( int addr )
	: Magnetometer()
	, mI2C9150( new I2C( addr ) )
	, mI2C( new I2C( 0x0C ) )
	, mState( 0 )
	, mData{ 0 }
{
	mNames = { "MPU6050" };

	uint8_t id = 0;
	int ret = mI2C->Read8( MPU_6050_WIA, &id );
	if ( ret < 0 or id != MPU_6050_AKM_ID ) {
		return;
	}

	// Single measurement mode: 0b00000001
	mI2C->Write8( MPU_6050_CNTL, 0b00000001 );
	usleep( 100 * 1000 );
}


MPU6050Accel::~MPU6050Accel()
{
}


MPU6050Gyro::~MPU6050Gyro()
{
}


MPU6050Mag::~MPU6050Mag()
{
}


void MPU6050Accel::Calibrate( float dt, bool last_pass )
{
	if ( mCalibrated ) {
		mCalibrated = false;
		mCalibrationAccum = Vector4f();
		mOffset = Vector3f();
	}

	Vector3f accel;
	Read( &accel, true );
	mCalibrationAccum += Vector4f( accel, 1.0f );

	if ( last_pass ) {
		mOffset = mCalibrationAccum.xyz() / mCalibrationAccum.w;
		mCalibrationAccum = Vector4f();
		mCalibrated = true;
		gDebug() << "MPU6050 SAVING CALIBRATED OFFSETS !\n";
		aDebug( "mOffset", mOffset.x, mOffset.y, mOffset.z );
		Board::SaveRegister( "MPU6050:Accelerometer:Offset:X", std::to_string( mOffset.x ) );
		Board::SaveRegister( "MPU6050:Accelerometer:Offset:Y", std::to_string( mOffset.y ) );
		Board::SaveRegister( "MPU6050:Accelerometer:Offset:Z", std::to_string( mOffset.z ) );
	}
}


void MPU6050Gyro::Calibrate( float dt, bool last_pass )
{
	if ( mCalibrated ) {
		mCalibrated = false;
		mCalibrationAccum = Vector4f();
		mOffset = Vector3f();
	}

	Vector3f gyro;
	Read( &gyro, true );
	mCalibrationAccum += Vector4f( gyro, 1.0f );

	if ( last_pass ) {
		mOffset = mCalibrationAccum.xyz() / mCalibrationAccum.w;
		mCalibrationAccum = Vector4f();
		mCalibrated = true;
	}
}


void MPU6050Mag::Calibrate( float dt, bool last_pass )
{
	// TODO

	mCalibrated = true;
}


void MPU6050Accel::Read( Vector3f* v, bool raw )
{
	int16_t curr[3] = { 0 };
	uint8_t saccel[6] = { 0 };

	mI2C->Read( MPU_6050_ACCEL_XOUT_H | 0x80, saccel, sizeof(saccel) );

	curr[0] = (int16_t)( saccel[0] << 8 | saccel[1] );
	curr[1] = (int16_t)( saccel[2] << 8 | saccel[3] );
	curr[2] = (int16_t)( saccel[4] << 8 | saccel[5] );

	v->x = (float)( curr[0] ) * 8.0f * 6.103515625e-04f;
	v->y = (float)( curr[1] ) * 8.0f * 6.103515625e-04f;
	v->z = (float)( curr[2] ) * 8.0f * 6.103515625e-04f;

	ApplySwap( *v );
	if ( not raw ) {
		v->x -= mOffset.x;
		v->y -= mOffset.y;
	}

	mLastValues = *v;
}


int MPU6050Gyro::Read( Vector3f* v, bool raw )
{
// 	int16_t sgyro[3] = { 0 };
	uint8_t sgyro[6] = { 0 };

	if ( mI2C->Read( MPU_6050_GYRO_XOUT_H | 0x80, sgyro, sizeof(sgyro) ) != sizeof(sgyro) ) {
		return -1;
	}
	v->x = (float)( (int16_t)( sgyro[0] << 8 | sgyro[1] ) ) * 0.061037018952f;
	v->y = (float)( (int16_t)( sgyro[2] << 8 | sgyro[3] ) ) * 0.061037018952f;
	v->z = (float)( (int16_t)( sgyro[4] << 8 | sgyro[5] ) ) * 0.061037018952f;

	ApplySwap( *v );
	if ( not raw ) {
		v->x -= mOffset.x;
		v->y -= mOffset.y;
		v->z -= mOffset.z;
	}

	mLastValues = *v;
	return 3;
}


void MPU6050Mag::Read( Vector3f* v, bool raw )
{
	switch( mState ) {
		case 0 : {
			// Check if data is ready.
			uint8_t status = 0;
			mI2C->Read8( MPU_6050_ST1, &status );
			if ( status & 0x01 ) {
				mState = 1;
			}
			// Duplicate last measurements
			*v = mLastValues;
			break;
		}
		
		case 1 : {
			// Read X axis
			for ( int i = 0; i < 2; i++ ) {
				mI2C->Read8( MPU_6050_HXL + i, &mData[i] );
			}
			mState = 2;

			// Duplicate last measurements
			*v = mLastValues;
			break;
		}
		
		case 2 : {
			// Read Y axis
			for ( int i = 2; i < 4; i++ ) {
				mI2C->Read8( MPU_6050_HXL + i, &mData[i] );
			}
			mState = 3;

			// Duplicate last measurements
			*v = mLastValues;
			break;
		}
		
		case 3 : {
			// Read Z axis
			for ( int i = 4; i < 6; i++ ) {
				mI2C->Read8( MPU_6050_HXL + i, &mData[i] );
			}

			v->x = (float)( (int16_t)( mData[1] << 8 | mData[0] ) ) * 0.3001221001221001f;
			v->y = (float)( (int16_t)( mData[3] << 8 | mData[2] ) ) * 0.3001221001221001f;
			v->z = (float)( (int16_t)( mData[5] << 8 | mData[4] ) ) * 0.3001221001221001f;

			mLastValues = *v;
			
			// Re-arm single measurement mode
			mI2C->Write8( MPU_6050_CNTL, 0b00000001 );
			mState = 0;
			break;
		}
		
		default : {
			mState = 0;
		}
	}
}


std::string MPU6050Gyro::infos()
{
	return "I2Caddr = " + std::to_string( mI2C->address() ) + ", " + "Resolution = \"16 bits\", " + "Scale = \"2000°/s\"";
}


std::string MPU6050Accel::infos()
{
	return "I2Caddr = " + std::to_string( mI2C->address() ) + ", " + "Resolution = \"16 bits\", " + "Scale = \"16g\"";
}


std::string MPU6050Mag::infos()
{
	return "I2Caddr = " + std::to_string( mI2C->address() ) + ", " + "Resolution = \"13 bits\", " + "Scale = \"1200μT\"";
}
