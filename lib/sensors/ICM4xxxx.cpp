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
#include "ICM4xxxx.h"
#include "Config.h"
#include "SPI.h"


static const map< uint8_t, string > known = {
	{ 0x42, "ICM-42605" }
};

static const map< uint8_t, bool > hasMagnetometer = {
	{ 0x42, false }
};


ICM4xxxx::ICM4xxxx()
	: Sensor()
	, mChipReady( false )
	, mGyroscope( nullptr )
	, mAccelerometer( nullptr )
	, mMagnetometer( nullptr )
	, dmpReady( false )
{
}


void ICM4xxxx::InitChip()
{

	uint8_t read_reg = 0;
	if ( dynamic_cast<SPI*>(mBus) != nullptr ) {
		read_reg = 0x80;
	}

	mBus->Connect();

	uint8_t whoami = 0x00;
	mBus->Read8( read_reg | ICM_4xxxx_WHO_AM_I, &whoami );
	gDebug() << "whoami : " << hex << showbase << (int)whoami;

	if ( known.find( whoami ) != known.end() ) {
		gDebug() << "ICM4xxxx module at " << mBus->toString() << " is " << known.at(whoami);
		mName = known.at(whoami);
	} else {
		gWarning() << "Unknown ICM4xxxx module at " << mBus->toString() << ", continuing anyway";
	}

	mBus->Write8( ICM_4xxxx_DEVICE_CONFIG, 0b00000001 );
	usleep( 1000 * 10 );
	mBus->Write8( ICM_4xxxx_DEVICE_CONFIG, 0b00000001 );
	usleep( 1000 * 10 );
	mBus->Write8( ICM_4xxxx_DEVICE_CONFIG, 0b00000001 );
	usleep( 1000 * 10 );

	// Disable all sensors while configuring
	mBus->Write8( ICM_4xxxx_PWR_MGMT0, 0b00000000 );

	// Can be tweaked to :
 	// 249Hz : (21, 440, 6)
	// 524Hz : (39, 1536, 4)
	// 995Hz : (63, 3968, 3)
	mBus->Write8( ICM_4xxxx_BANK_SEL, ICM_4xxxx_BANK_SELECT1 );
	mBus->Write8( ICM_4xxxx_GYRO_CONFIG_STATIC3, 21 );
	mBus->Write8( ICM_4xxxx_GYRO_CONFIG_STATIC4, 440 & 0xFF );
	mBus->Write8( ICM_4xxxx_GYRO_CONFIG_STATIC5, (440 >> 8) | (6 << 4) );

	// Fixed values for accelerometer
	mBus->Write8( ICM_4xxxx_BANK_SEL, ICM_4xxxx_BANK_SELECT2 );
	mBus->Write8( ICM_4xxxx_ACCEL_CONFIG_STATIC2, 21 );
	mBus->Write8( ICM_4xxxx_ACCEL_CONFIG_STATIC3, 440 & 0xFF );
	mBus->Write8( ICM_4xxxx_ACCEL_CONFIG_STATIC4, (440 >> 8) | (6 << 4) );

	mBus->Write8( ICM_4xxxx_BANK_SEL, ICM_4xxxx_BANK_SELECT0 );

	// Accel & Gyro LPF : Low-latency
	mBus->Write8( ICM_4xxxx_GYRO_ACCEL_CONFIG0, ( 14 << 4 ) | 14 );
	// mBus->Write8( ICM_4xxxx_GYRO_ACCEL_CONFIG0, ( 7 << 4 ) | 7 );


	// mBus->Write8( ICM_4xxxx_INTF_CONFIG0, 0b00000011 );
	mBus->Write8( ICM_4xxxx_INTF_CONFIG0, 0b00110000 );
	// uint8_t config1 = 0;
	// mBus->Read8( ICM_4xxxx_INTF_CONFIG1, &config1 );
    // config1 &= ~0xC0; // AFSR mask
    // config1 |= 0x40; // AFSR disable
	// mBus->Write8( ICM_4xxxx_INTF_CONFIG1, config1 );
	mBus->Write8( ICM_4xxxx_INTF_CONFIG6, 0b01010011 );
	mBus->Write8( ICM_4xxxx_DRIVE_CONFIG, 0b00000101 );
	mBus->Write8( ICM_4xxxx_FIFO_CONFIG, 0b00000000 );
	mBus->Write8( ICM_4xxxx_INT_CONFIG0, 0b00011011 );
	mBus->Write8( ICM_4xxxx_INT_CONFIG1, 0b01100000 );

	// Disable all interrupts
	mBus->Write8( ICM_4xxxx_INT_SOURCE0, 0b00001000 );
	mBus->Write8( ICM_4xxxx_INT_SOURCE1, 0b00000000 );
	mBus->Write8( ICM_4xxxx_INT_SOURCE3, 0b00000000 );
	mBus->Write8( ICM_4xxxx_INT_SOURCE4, 0b00000000 );
	// Disable all FIFOs: 0b00000000
	mBus->Write8( ICM_4xxxx_FIFO_CONFIG1, 0b00000000 );
	mBus->Write8( ICM_4xxxx_FIFO_CONFIG2, 0b00000000 );
	mBus->Write8( ICM_4xxxx_FIFO_CONFIG3, 0b00000000 );

	// Enable all sensors in low-noise mode + never go idle
	mBus->Write8( ICM_4xxxx_PWR_MGMT0, 0b00011111 );

	uint32_t loopTime = Main::instance()->config()->Integer( "stabilizer.loop_time", 2000 );
	uint32_t loopRate = 1000000 / loopTime;
	const std::list< std::tuple< uint32_t, uint8_t > > rates = {
		{   25, 0b00001010 },
		{   50, 0b00001001 },
		{  100, 0b00001000 },
		{  200, 0b00000111 },
		{  500, 0b00001111 },
		{ 1000, 0b00000110 },
		{ 2000, 0b00000101 },
		{ 4000, 0b00000100 },
		{ 8000, 0b00000011 },
	};
	uint8_t rate = 0b00000000;
	for ( auto r : rates ) {
		if ( loopRate <= std::get<0>(r) ) {
			rate = std::get<1>(r);
			break;
		}
	}
	gDebug() << "Setting ICM4xxxx rate to " << loopRate << "Hz (" << (int)rate << ")";
	mBus->Write8( ICM_4xxxx_GYRO_CONFIG0, rate );
	mBus->Write8( ICM_4xxxx_ACCEL_CONFIG0, rate );

	mChipReady = true;
}


ICM4xxxxGyro* ICM4xxxx::gyroscope()
{
	if ( !mChipReady ) {
		InitChip();
	}
	if ( !mGyroscope ) {
		mGyroscope = new ICM4xxxxGyro( mBus, mName );
		if ( mSwapMode == SwapModeAxis ) {
			mGyroscope->setAxisSwap(mAxisSwap);
		}
		if ( mSwapMode == SwapModeMatrix ) {
			mGyroscope->setAxisMatrix(mAxisMatrix);
		}
	}
	return mGyroscope;
}


ICM4xxxxAccel* ICM4xxxx::accelerometer()
{
	if ( !mChipReady ) {
		InitChip();
	}
	if ( !mAccelerometer ) {
		mAccelerometer = new ICM4xxxxAccel( mBus, mName );
		if ( mSwapMode == SwapModeAxis ) {
			mAccelerometer->setAxisSwap(mAxisSwap);
		}
		if ( mSwapMode == SwapModeMatrix ) {
			mAccelerometer->setAxisMatrix(mAxisMatrix);
		}
	}
	return mAccelerometer;
}


ICM4xxxxMag* ICM4xxxx::magnetometer()
{
	if ( !mChipReady ) {
		InitChip();
	}
	if ( !mMagnetometer and hasMagnetometer.at( mWhoAmI ) ) {
		mMagnetometer = new ICM4xxxxMag( mBus, mName );
		if ( mSwapMode == SwapModeAxis ) {
			mMagnetometer->setAxisSwap(mAxisSwap);
		}
		if ( mSwapMode == SwapModeMatrix ) {
			mMagnetometer->setAxisMatrix(mAxisMatrix);
		}
	}
	return mMagnetometer;
}


void ICM4xxxx::setGyroscopeAxisSwap( const Vector4i& swap )
{
	gyroscope()->setAxisSwap( swap );
}


void ICM4xxxx::setAccelerometerAxisSwap( const Vector4i& swap )
{
	accelerometer()->setAxisSwap( swap );
}


void ICM4xxxx::setMagnetometerAxisSwap( const Vector4i& swap )
{
	magnetometer()->setAxisSwap( swap );
}


ICM4xxxxAccel::ICM4xxxxAccel( Bus* bus, const std::string& name )
	: Accelerometer()
	, mBus( bus )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { name };

	mOffset.x = atof( Board::LoadRegister( "ICM4xxxx<" + mBus->toString() + ">:Accelerometer:Offset:X" ).c_str() );
	mOffset.y = atof( Board::LoadRegister( "ICM4xxxx<" + mBus->toString() + ">:Accelerometer:Offset:Y" ).c_str() );
	mOffset.z = atof( Board::LoadRegister( "ICM4xxxx<" + mBus->toString() + ">:Accelerometer:Offset:Z" ).c_str() );
	if ( mOffset.x != 0.0f and mOffset.y != 0.0f and mOffset.z != 0.0f ) {
		mCalibrated = true;
	}
}


ICM4xxxxGyro::ICM4xxxxGyro( Bus* bus, const std::string& name )
	: Gyroscope()
	, mBus( bus )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { name };
}


ICM4xxxxMag::ICM4xxxxMag( Bus* bus, const std::string& name )
	: Magnetometer()
// 	, mI2C9150( new I2C( addr ) )
// 	, mI2C( new I2C( ICM_4xxxx_I2C_MAGN_ADDRESS ) )
	, mBus( bus )
	, mState( 0 )
	, mData{ 0 }
	, mCalibrationData{ 0.0f }
	, mBias{ 0.0f }
	, mBiasMin{ 32767 }
	, mBiasMax{ -32767 }
{
	mNames = { name };

	// TODO : talk to mag accross SPI if using SPI
	// TODO
/*
	uint8_t id = 0;
	int ret = mBus->Read8( ICM_4xxxx_WIA | 0x80, &id );
	if ( ret < 0 or id != ICM_4xxxx_AKM_ID ) {
		return;
	}

	mBus->Write8( ICM_4xxxx_CNTL, 0x00 );
	usleep( 1000 * 10 );
	mBus->Write8( ICM_4xxxx_CNTL, 0x0F ); // Enter Fuse ROM access mode
	usleep( 1000 * 10 );
	uint8_t rawData[3];  // x/y/z gyro calibration data stored here
	mBus->Read( AK8963_ASAX, rawData, 3 );
	mCalibrationData[0] = (float)(rawData[0] - 128)/256.0f + 1.0f;
	mCalibrationData[1] = (float)(rawData[1] - 128)/256.0f + 1.0f;
	mCalibrationData[2] = (float)(rawData[2] - 128)/256.0f + 1.0f;
	mBus->Write8( ICM_4xxxx_CNTL, 0x00 );
	// Configure the magnetometer for continuous read and highest resolution
	// set Mscale bit 4 to 1 (0) to enable 16 (14) bit resolution in CNTL register,
	// and enable continuous mode data acquisition Mmode (bits [3:0]), 0010 for 8 Hz and 0110 for 100 Hz sample rates
	mBus->Write8( ICM_4xxxx_CNTL, 0x01 << 4 | 0x06 ); // Set magnetometer data resolution and sample ODR

	// Single measurement mode: 0b00000001
// 	mBus->Write8( ICM_4xxxx_CNTL, 0b00000001 );
	usleep( 100 * 1000 );
*/
}


ICM4xxxxAccel::~ICM4xxxxAccel()
{
}


ICM4xxxxGyro::~ICM4xxxxGyro()
{
}


ICM4xxxxMag::~ICM4xxxxMag()
{
}


void ICM4xxxxAccel::Calibrate( float dt, bool last_pass )
{
	if ( mCalibrated ) {
		mCalibrated = false;
		mCalibrationAccum = Vector4f();
		mOffset = Vector3f();
	}

	Vector3f accel( 0.0f, 0.0f, 0.0f );
	Read( &accel, true );
	mCalibrationAccum += Vector4f( accel, 1.0f );

	if ( last_pass ) {
		mOffset = mCalibrationAccum.xyz() / mCalibrationAccum.w;
		mCalibrationAccum = Vector4f();
		mCalibrated = true;
		gDebug() << "ICM4xxxx SAVING CALIBRATED OFFSETS !";
		aDebug( "mOffset", mOffset.x, mOffset.y, mOffset.z );
		Board::SaveRegister( "ICM4xxxx<" + mBus->toString() + ">:Accelerometer:Offset:X", to_string( mOffset.x ) );
		Board::SaveRegister( "ICM4xxxx<" + mBus->toString() + ">:Accelerometer:Offset:Y", to_string( mOffset.y ) );
		Board::SaveRegister( "ICM4xxxx<" + mBus->toString() + ">:Accelerometer:Offset:Z", to_string( mOffset.z ) );
	}
}


void ICM4xxxxGyro::Calibrate( float dt, bool last_pass )
{
	if ( mCalibrated ) {
		mCalibrated = false;
		mCalibrationAccum = Vector4f();
		mOffset = Vector3f();
	}

	Vector3f gyro( 0.0f, 0.0f, 0.0f );
	Read( &gyro, true );
	mCalibrationAccum += Vector4f( gyro, 1.0f );

	if ( last_pass ) {
		mOffset = mCalibrationAccum.xyz() / mCalibrationAccum.w;
		mCalibrationAccum = Vector4f();
		mCalibrated = true;
		aDebug( "mOffset", mOffset.x, mOffset.y, mOffset.z );
	}
}


void ICM4xxxxMag::Calibrate( float dt, bool last_pass )
{
	mCalibrated = true;
/*
	const float MPU9250mRes = 10.0f * 4912.0f / 32760.0f;
	int16_t raw_vec[3];
	uint8_t state = 0;
	uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition

	mBus->Read8( ICM_4xxxx_ST1, &state );
	if ( state & 0x01 ) { // wait for magnetometer data ready bit to be set
		mBus->Read( ICM_4xxxx_HXL, rawData, 7 );  // Read the six raw data and ST2 registers sequentially into data array
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
*/
}


void ICM4xxxxAccel::Read( Vector3f* v, bool raw )
{
	int16_t curr[3] = { 0 };
	uint8_t saccel[6] = { 0 };

	int r = mBus->Read( ICM_4xxxx_ACCEL_DATA_X1 | 0x80, saccel, sizeof(saccel) );

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

int ICM4xxxxGyro::Read( Vector3f* v, bool raw )
{
	// fDebug();

// 	int16_t sgyro[3] = { 0 };
	uint8_t sgyro[6] = { 0 };
	int ret = 0;

	ret = mBus->Read( ICM_4xxxx_GYRO_DATA_X1 | 0x80, sgyro, sizeof(sgyro) );
	if ( ret != sizeof(sgyro) ) {
		gWarning() << "read " << ret << " bytes instead of " << sizeof(sgyro);
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
	// gDebug() << "ret 3";
	return 3;
}


void ICM4xxxxMag::Read( Vector3f* v, bool raw )
{/*
	float mx, my, mz;
	const float MPU9250mRes = 10.0f * 4912.0f / 32760.0f;
	int16_t raw_vec[3];
	uint8_t state = 0;
	uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition

	mBus->Read8( ICM_4xxxx_ST1, &state );
	if ( state & 0x01 ) { // wait for magnetometer data ready bit to be set
		mBus->Read( ICM_4xxxx_HXL, rawData, 7 );  // Read the six raw data and ST2 registers sequentially into data array
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
*/
/*
	switch( mState ) {
		case 0 : {
			// Check if data is ready.
			uint8_t status = 0;
			mBus->Read8( ICM_4xxxx_ST1, &status );
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
				mBus->Read8( ICM_4xxxx_HXL + i, &mData[i] );
			}
			mState = 2;

			// Duplicate last measurements
			*v = mLastValues;
			break;
		}
		
		case 2 : {
			// Read Y axis
			for ( int i = 2; i < 4; i++ ) {
				mBus->Read8( ICM_4xxxx_HXL + i, &mData[i] );
			}
			mState = 3;

			// Duplicate last measurements
			*v = mLastValues;
			break;
		}
		
		case 3 : {
			// Read Z axis
			for ( int i = 4; i < 6; i++ ) {
				mBus->Read8( ICM_4xxxx_HXL + i, &mData[i] );
			}

			v->x = (float)( (int16_t)( mData[1] << 8 | mData[0] ) ) * 0.3001221001221001f;
			v->y = (float)( (int16_t)( mData[3] << 8 | mData[2] ) ) * 0.3001221001221001f;
			v->z = (float)( (int16_t)( mData[5] << 8 | mData[4] ) ) * 0.3001221001221001f;

			mLastValues = *v;
			
			// Re-arm single measurement mode
			mBus->Write8( ICM_4xxxx_CNTL, 0b00000001 );
			mState = 0;
			break;
		}
		
		default : {
			mState = 0;
		}
	}
*/
}


LuaValue ICM4xxxxGyro::infos()
{
	LuaValue ret;

	ret["bus"] = mBus->infos();
	// TODO : read from chip
	ret["precision"] = "16 bits";
	ret["scale"] = "2000°/s";
	ret["sample_rate"] = "1kHz";
	ret["swap_axis"] = swapInfos();

	return ret;
}


LuaValue ICM4xxxxAccel::infos()
{
	LuaValue ret;

	ret["bus"] = mBus->infos();
	// TODO : read from chip
	ret["precision"] = "16 bits";
	ret["scale"] = "16g";
	ret["sample_rate"] = "1kHz";
	ret["swap_axis"] = swapInfos();

	return ret;
}


LuaValue ICM4xxxxMag::infos()
{
	return "";
// TODO
}
