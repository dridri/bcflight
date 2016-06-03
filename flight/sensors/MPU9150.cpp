#include <unistd.h>
#include "MPU9150.h"

int MPU9150::flight_register( Main* main )
{
	Device dev1;
	Device dev2;

	dev1.iI2CAddr = 0x68;
	dev1.name = "MPU9150";
	dev1.fInstanciate = MPU9150::Instanciate;

	dev1.iI2CAddr = 0x69;
	dev1.name = "MPU9150";
	dev1.fInstanciate = MPU9150::Instanciate;

	mKnownDevices.push_back( dev1 );
	mKnownDevices.push_back( dev2 );
	return 0;
}


Sensor* MPU9150::Instanciate( Config* config, const std::string& object )
{
	// TODO : check if address is 0x68 or 0x69
	int i2c_addr = 0x69;
	I2C* i2c = new I2C( i2c_addr );

	// No power management, internal clock source: 0b00000000
	i2c->Write8( MPU_9150_PWR_MGMT_1, 0b00000000 );
	// Disable all interrupts: 0b00000000
	i2c->Write8( MPU_9150_INT_ENABLE, 0b00000000 );
	// Disable all FIFOs: 0b00000000
	i2c->Write8( MPU_9150_FIFO_EN, 0b00000000 );
	// 1 kHz sampling rate: 0b00000000
	i2c->Write8( MPU_9150_SMPRT_DIV, 0b00000000 );
	// No ext sync, DLPF at 94Hz for the accel and 98Hz -0b00000010) for the gyro: 0b00000010 (~200Hz: 0b00000001)
	i2c->Write8( MPU_9150_DEFINE, 0b00000000 );
	// Gyro range at +/-2000 °/s
	i2c->Write8( MPU_9150_GYRO_CONFIG, 0b00011000 );
	// Accel range at +/-16g
	i2c->Write8( MPU_9150_ACCEL_CONFIG, 0b00011000 );
	// Bypass mode enabled: 0b00000010
	i2c->Write8( MPU_9150_INT_PIN_CFG, 0b00000010 );
	// No FIFO and no I2C slaves: 0b00000000
	i2c->Write8( MPU_9150_USER_CTRL, 0b00000000 );

	delete i2c;


	// Manually add sensors to mDevices since they use the same address
	Sensor* mag = new MPU9150Mag( i2c_addr );
	Sensor* gyro = new MPU9150Gyro( i2c_addr );
	Sensor* accel = new MPU9150Accel( i2c_addr );
	mDevices.push_back( accel );
	mDevices.push_back( gyro );
	mDevices.push_back( mag );

	return nullptr;
}


MPU9150Accel::MPU9150Accel( int addr )
	: Accelerometer()
	, mI2C( new I2C( addr ) )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { "MPU9150" };

	mOffset.x = std::atof( Board::LoadRegister( "MPU9150:Accelerometer:Offset:X" ).c_str() );
	mOffset.y = std::atof( Board::LoadRegister( "MPU9150:Accelerometer:Offset:Y" ).c_str() );
	mOffset.z = std::atof( Board::LoadRegister( "MPU9150:Accelerometer:Offset:Z" ).c_str() );
	if ( mOffset.x != 0.0f and mOffset.y != 0.0f and mOffset.z != 0.0f ) {
		mCalibrated = true;
	}
}


MPU9150Gyro::MPU9150Gyro( int addr )
	: Gyroscope()
	, mI2C( new I2C( addr ) )
	, mCalibrationAccum( Vector4f() )
	, mOffset( Vector3f() )
{
	mNames = { "MPU9150" };
}


MPU9150Mag::MPU9150Mag( int addr )
	: Magnetometer()
	, mI2C9150( new I2C( addr ) )
	, mI2C( new I2C( 0x0C ) )
	, mState( 0 )
	, mData{ 0 }
{
	mNames = { "MPU9150" };

	uint8_t id = 0;
	int ret = mI2C->Read8( MPU_9150_WIA, &id );
	if ( ret < 0 or id != MPU_9150_AKM_ID ) {
		return;
	}

	// Single measurement mode: 0b00000001
	mI2C->Write8( MPU_9150_CNTL, 0b00000001 );
	usleep( 100 * 1000 );
}


MPU9150Accel::~MPU9150Accel()
{
}


MPU9150Gyro::~MPU9150Gyro()
{
}


MPU9150Mag::~MPU9150Mag()
{
}


void MPU9150Accel::Calibrate( float dt, bool last_pass )
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
		gDebug() << "MPU9150 SAVING CALIBRATED OFFSETS !\n";
		aDebug( "mOffset", mOffset.x, mOffset.y, mOffset.z );
		Board::SaveRegister( "MPU9150:Accelerometer:Offset:X", std::to_string( mOffset.x ) );
		Board::SaveRegister( "MPU9150:Accelerometer:Offset:Y", std::to_string( mOffset.y ) );
		Board::SaveRegister( "MPU9150:Accelerometer:Offset:Z", std::to_string( mOffset.z ) );
	}
}


void MPU9150Gyro::Calibrate( float dt, bool last_pass )
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


void MPU9150Mag::Calibrate( float dt, bool last_pass )
{
	// TODO
}


void MPU9150Accel::Read( Vector3f* v, bool raw )
{
// 	short saccel[3] = { 0 };
	uint8_t saccel[6] = { 0 };

	mI2C->Read( MPU_9150_ACCEL_XOUT_H | 0x80, saccel, sizeof(saccel) );

	v->x = (float)( (int16_t)( saccel[0] << 8 | saccel[1] ) ) * 8.0f * 6.103515625e-04f;
	v->y = (float)( (int16_t)( saccel[2] << 8 | saccel[3] ) ) * 8.0f * 6.103515625e-04f;
	v->z = (float)( (int16_t)( saccel[4] << 8 | saccel[5] ) ) * 8.0f * 6.103515625e-04f;

	ApplySwap( *v );
	if ( not raw ) {
		v->x -= mOffset.x;
		v->y -= mOffset.y;
	}

	mLastValues = *v;
}


void MPU9150Gyro::Read( Vector3f* v, bool raw )
{
// 	short sgyro[3] = { 0 };
	uint8_t sgyro[6] = { 0 };

	mI2C->Read( MPU_9150_GYRO_XOUT_H | 0x80, sgyro, sizeof(sgyro) );
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
}


void MPU9150Mag::Read( Vector3f* v, bool raw )
{
	switch( mState ) {
		case 0 : {
			// Check if data is ready.
			uint8_t status = 0;
			mI2C->Read8( MPU_9150_ST1, &status );
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
				mI2C->Read8( MPU_9150_HXL + i, &mData[i] );
			}
			mState = 2;

			// Duplicate last measurements
			*v = mLastValues;
			break;
		}
		
		case 2 : {
			// Read Y axis
			for ( int i = 2; i < 4; i++ ) {
				mI2C->Read8( MPU_9150_HXL + i, &mData[i] );
			}
			mState = 3;

			// Duplicate last measurements
			*v = mLastValues;
			break;
		}
		
		case 3 : {
			// Read Z axis
			for ( int i = 4; i < 6; i++ ) {
				mI2C->Read8( MPU_9150_HXL + i, &mData[i] );
			}

			v->x = (float)( (int16_t)( mData[1] << 8 | mData[0] ) ) * 0.3001221001221001f;
			v->y = (float)( (int16_t)( mData[3] << 8 | mData[2] ) ) * 0.3001221001221001f;
			v->z = -(float)( (int16_t)( mData[5] << 8 | mData[4] ) ) * 0.3001221001221001f;

			mLastValues = *v;
			
			// Re-arm single measurement mode
			mI2C->Write8( MPU_9150_CNTL, 0b00000001 );
			mState = 0;
			break;
		}
		
		default : {
			mState = 0;
		}
	}
}


std::string MPU9150Gyro::infos()
{
	return "I2C address = " + std::to_string( mI2C->address() ) + ", " + "Resolution = \"16 bits\", " + "Scale = \"2000°/s\"";
}


std::string MPU9150Accel::infos()
{
	return "I2C address = " + std::to_string( mI2C->address() ) + ", " + "Resolution = \"16 bits\", " + "Scale = \"16g\"";
}


std::string MPU9150Mag::infos()
{
	return "I2C address = " + std::to_string( mI2C->address() ) + ", " + "Resolution = \"13 bits\", " + "Scale = \"1200μT\"";
}
