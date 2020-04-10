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
	dev1.fInstanciate = []( Config* config, const string& object ) { return MPU6050::Instanciate( 0x68, config, object ); };

	dev2.iI2CAddr = 0x69;
	dev2.name = "MPU6050";
	dev2.fInstanciate = []( Config* config, const string& object ) { return MPU6050::Instanciate( 0x69, config, object ); };

	mKnownDevices.push_back( dev1 );
	mKnownDevices.push_back( dev2 );
	return 0;
}


Sensor* MPU6050::Instanciate( uint8_t i2c_addr, Config* config, const string& object )
{
	I2C* i2c = new I2C( i2c_addr );

	uint8_t whoami = 0x00;
	i2c->Read8( MPU_6050_WHO_AM_I, &whoami );
	bool mpu9250 = ( whoami == 0x71 or whoami == 0x73 );
	bool icm20608 = ( whoami == 0xAF );

	gDebug() << "whoami [0x" << hex << (int)i2c_addr << "]0x" << hex << (int)whoami << "\n";

	if ( mpu9250 ) {
		gDebug() << "I2C module at address 0x" << hex << (int)i2c->address() << " is MPU9250\n";
	} else if ( icm20608 ) {
		gDebug() << "I2C module at address 0x" << hex << (int)i2c->address() << " is ICM28608\n";
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

	mOffset.x = atof( Board::LoadRegister( "MPU6050:Accelerometer:Offset:X" ).c_str() );
	mOffset.y = atof( Board::LoadRegister( "MPU6050:Accelerometer:Offset:Y" ).c_str() );
	mOffset.z = atof( Board::LoadRegister( "MPU6050:Accelerometer:Offset:Z" ).c_str() );
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
	, mI2C( new I2C( MPU_6050_I2C_MAGN_ADDRESS ) )
	, mState( 0 )
	, mData{ 0 }
	, mCalibrationData{ 0.0f }
	, mBias{ 0.0f }
	, mBiasMin{ 32767 }
	, mBiasMax{ -32767 }
{
	mNames = { "MPU6050" };

	uint8_t id = 0;
	int ret = mI2C->Read8( MPU_6050_WIA, &id );
	if ( ret < 0 or id != MPU_6050_AKM_ID ) {
		return;
	}

	mI2C->Write8( MPU_6050_CNTL, 0x00 );
	usleep( 1000 * 10 );
	mI2C->Write8( MPU_6050_CNTL, 0x0F ); // Enter Fuse ROM access mode
	usleep( 1000 * 10 );
	uint8_t rawData[3];  // x/y/z gyro calibration data stored here
	mI2C->Read( AK8963_ASAX, rawData, 3 );
	mCalibrationData[0] = (float)(rawData[0] - 128)/256.0f + 1.0f;
	mCalibrationData[1] = (float)(rawData[1] - 128)/256.0f + 1.0f;
	mCalibrationData[2] = (float)(rawData[2] - 128)/256.0f + 1.0f;
	mI2C->Write8( MPU_6050_CNTL, 0x00 );
	// Configure the magnetometer for continuous read and highest resolution
	// set Mscale bit 4 to 1 (0) to enable 16 (14) bit resolution in CNTL register,
	// and enable continuous mode data acquisition Mmode (bits [3:0]), 0010 for 8 Hz and 0110 for 100 Hz sample rates
	mI2C->Write8( MPU_6050_CNTL, 0x01 << 4 | 0x06 ); // Set magnetometer data resolution and sample ODR

	// Single measurement mode: 0b00000001
// 	mI2C->Write8( MPU_6050_CNTL, 0b00000001 );
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
		Board::SaveRegister( "MPU6050:Accelerometer:Offset:X", to_string( mOffset.x ) );
		Board::SaveRegister( "MPU6050:Accelerometer:Offset:Y", to_string( mOffset.y ) );
		Board::SaveRegister( "MPU6050:Accelerometer:Offset:Z", to_string( mOffset.z ) );
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
	const float MPU9250mRes = 10.0f * 4912.0f / 32760.0f;
	int16_t raw_vec[3];
	uint8_t state = 0;
	uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition

	mI2C->Read8( MPU_6050_ST1, &state );
	if ( state & 0x01 ) { // wait for magnetometer data ready bit to be set
		mI2C->Read( MPU_6050_HXL, rawData, 7 );  // Read the six raw data and ST2 registers sequentially into data array
		uint8_t c = rawData[6]; // End data read by reading ST2 register
		if ( !( c & 0x08 ) ) { // Check if magnetic sensor overflow set, if not then report data
			raw_vec[0] = ((int16_t)rawData[1] << 8) | rawData[0];  // Turn the MSB and LSB into a signed 16-bit value
			raw_vec[1] = ((int16_t)rawData[3] << 8) | rawData[2];  // Data stored as little Endian
			raw_vec[2] = ((int16_t)rawData[5] << 8) | rawData[4];
			for ( uint32_t i = 0; i < 3; i++ ) {
				mBiasMax[i] = max( mBiasMax[i], raw_vec[i] );
				mBiasMin[i] = min( mBiasMin[i], raw_vec[i] );
			}
		}
	}

	if ( last_pass ) {
		mBias[0] = (float)( ( mBiasMax[0] + mBiasMin[0] ) / 2 ) * MPU9250mRes * mCalibrationData[0];
		mBias[1] = (float)( ( mBiasMax[1] + mBiasMin[1] ) / 2 ) * MPU9250mRes * mCalibrationData[1];
		mBias[2] = (float)( ( mBiasMax[2] + mBiasMin[2] ) / 2 ) * MPU9250mRes * mCalibrationData[2];
		mCalibrated = true;
		printf( "mBias : %.2f, %.2f, %.2f\n", mBias[0], mBias[1], mBias[2] );
	}
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
	float mx, my, mz;
	const float MPU9250mRes = 10.0f * 4912.0f / 32760.0f;
	int16_t raw_vec[3];
	uint8_t state = 0;
	uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition

	mI2C->Read8( MPU_6050_ST1, &state );
	if ( state & 0x01 ) { // wait for magnetometer data ready bit to be set
		mI2C->Read( MPU_6050_HXL, rawData, 7 );  // Read the six raw data and ST2 registers sequentially into data array
		uint8_t c = rawData[6]; // End data read by reading ST2 register
		if ( !( c & 0x08 ) ) { // Check if magnetic sensor overflow set, if not then report data
			raw_vec[0] = ((int16_t)rawData[1] << 8) | rawData[0];  // Turn the MSB and LSB into a signed 16-bit value
			raw_vec[1] = ((int16_t)rawData[3] << 8) | rawData[2];  // Data stored as little Endian
			raw_vec[2] = ((int16_t)rawData[5] << 8) | rawData[4];
		}
	}

	mx = (float)raw_vec[0] * MPU9250mRes * mCalibrationData[0] - mBias[0];
	my = (float)raw_vec[1] * MPU9250mRes * mCalibrationData[1] - mBias[1];
	mz = (float)raw_vec[2] * MPU9250mRes * mCalibrationData[2] - mBias[2];
	v->x = mx;
	v->y = my;
	v->z = mz;
/*
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
*/
}


string MPU6050Gyro::infos()
{
	return "I2Caddr = " + to_string( mI2C->address() ) + ", " + "Resolution = \"16 bits\", " + "Scale = \"2000°/s\"";
}


string MPU6050Accel::infos()
{
	return "I2Caddr = " + to_string( mI2C->address() ) + ", " + "Resolution = \"16 bits\", " + "Scale = \"16g\"";
}


string MPU6050Mag::infos()
{
	return "I2Caddr = " + to_string( mI2C->address() ) + ", " + "Resolution = \"13 bits\", " + "Scale = \"1200μT\"";
}
