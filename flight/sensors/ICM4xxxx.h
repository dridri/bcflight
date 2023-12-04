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

#ifndef ICM4xxxx_H
#define ICM4xxxx_H

#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <I2C.h>
#include <Quaternion.h>


class ICM4xxxxAccel : public Accelerometer
{
public:
	ICM4xxxxAccel( Bus* bus, const std::string& name ="ICM4xxxx" );
	~ICM4xxxxAccel();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	LuaValue infos();

private:
	Bus* mBus;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


class ICM4xxxxGyro : public Gyroscope
{
public:
	ICM4xxxxGyro( Bus* bus, const std::string& name = "ICM4xxxx" );
	~ICM4xxxxGyro();

	void Calibrate( float dt, bool last_pass = false );
	int Read( Vector3f* v, bool raw = false );

	LuaValue infos();

private:
	Bus* mBus;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


class ICM4xxxxMag : public Magnetometer
{
public:
	ICM4xxxxMag( Bus* bus, const std::string& name = "ICM4xxxx" );
	~ICM4xxxxMag();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	LuaValue infos();

private:
	I2C* mI2C9150;
	Bus* mBus;
	int mState;
	uint8_t mData[6];
	float mCalibrationData[3];
	float mBias[3];
	int16_t mBiasMin[3];
	int16_t mBiasMax[3];
};


LUA_CLASS class ICM4xxxx : public Sensor
{
public:
	LUA_EXPORT ICM4xxxx();
	LUA_PROPERTY() ICM4xxxxGyro* gyroscope();
	LUA_PROPERTY() ICM4xxxxAccel* accelerometer();
	LUA_PROPERTY() ICM4xxxxMag* magnetometer();
	void InitChip();

	LUA_PROPERTY("gyroscope_axis_swap") void setGyroscopeAxisSwap( const Vector4i& swap );
	LUA_PROPERTY("accelerometer_axis_swap") void setAccelerometerAxisSwap( const Vector4i& swap );
	LUA_PROPERTY("magnetometer_axis_swap") void setMagnetometerAxisSwap( const Vector4i& swap );

protected:
	ICM4xxxx( I2C* i2c );
	uint8_t dmpInitialize();
	uint8_t dmpGetAccel(int32_t* data, const uint8_t* packet);
	uint8_t dmpGetAccel(int16_t* data, const uint8_t* packet);
	uint8_t dmpGetAccel(Vector3f* v, const uint8_t* packet);
	uint8_t dmpGetQuaternion(int32_t* data, const uint8_t* packet);
	uint8_t dmpGetQuaternion(int16_t* data, const uint8_t* packet);
	uint8_t dmpGetQuaternion(Quaternion* q, const uint8_t* packet);
	uint8_t dmpGetGyro(int32_t* data, const uint8_t* packet);
	uint8_t dmpGetGyro(int16_t* data, const uint8_t* packet);
	uint8_t dmpGetMag(int16_t* data, const uint8_t* packet);
	uint8_t dmpGetLinearAccel(Vector3f* v, Vector3f* vRaw, Vector3f* gravity);
	uint8_t dmpGetLinearAccelInWorld(Vector3f* v, Vector3f* vReal, Quaternion* q);
	uint8_t dmpGetGravity(Vector3f* v, Quaternion* q);
	uint8_t dmpGetEuler(float* data, Quaternion* q);
	uint8_t dmpGetYawPitchRoll(float* data, Quaternion* q, Vector3f* gravity);
	uint8_t dmpProcessFIFOPacket(const unsigned char* dmpData);
	uint8_t dmpReadAndProcessFIFOPacket(uint8_t numPackets, uint8_t* processed);
	uint16_t dmpGetFIFOPacketSize();

	void resetFIFO();
	uint16_t getFIFOCount();
	uint8_t getFIFOByte();
	void getFIFOBytes( uint8_t *data, uint8_t length );
	void setFIFOByte( uint8_t data );
	uint8_t readMemoryByte();
	int8_t getXGyroOffset();
	int8_t getYGyroOffset();
	int8_t getZGyroOffset();
	void setXGyroOffsetUser( int16_t offset );
	void setYGyroOffsetUser( int16_t offset );
	void setZGyroOffsetUser( int16_t offset );
	void setXGyroOffsetTC( char offset );
	void setYGyroOffsetTC( char offset );
	void setZGyroOffsetTC( char offset );
	void setMemoryBank( uint8_t bank, uint8_t prefetchEnabled, uint8_t userBank );
	void setMemoryStartAddress( uint8_t address );
	uint8_t writeProgMemoryBlock( const uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address, uint8_t verify );
	uint8_t writeProgDMPConfigurationSet( const uint8_t *data, uint16_t dataSize );
	uint8_t writeDMPConfigurationSet( const uint8_t *data, uint16_t dataSize, uint8_t useProgMem );
	void readMemoryBlock( uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address );
	uint8_t writeMemoryBlock( const uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address, uint8_t verify, uint8_t useProgMem );
	bool writeBit( uint8_t regAddr, uint8_t bitNum, uint8_t data );
	bool writeBits( uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data );
	char readBits( uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data );
	uint8_t getIntStatus();

	bool mChipReady;
	std::string mName;
	ICM4xxxxGyro* mGyroscope;
	ICM4xxxxAccel* mAccelerometer;
	ICM4xxxxMag* mMagnetometer;
	bool dmpReady;

	void Calibrate( float dt, bool last_pass = false ) {}
	void Read( Vector3f* v, bool raw = false ) {}
};

// 76

#define ICM_4xxxx_WHO_AM_I				0x75
#define ICM_4xxxx_DEVICE_CONFIG			0x11
#define ICM_4xxxx_DRIVE_CONFIG			0x13
#define ICM_4xxxx_INTF_CONFIG0			0x4C
#define ICM_4xxxx_INTF_CONFIG1			0x4D
#define ICM_4xxxx_INTF_CONFIG6			0x7C
#define ICM_4xxxx_INT_STATUS			0x2D
#define ICM_4xxxx_PWR_MGMT0				0x4E
#define ICM_4xxxx_GYRO_CONFIG0			0x4F
#define ICM_4xxxx_GYRO_CONFIG1			0x51
#define ICM_4xxxx_GYRO_ACCEL_CONFIG0	0x52
#define ICM_4xxxx_ACCEL_CONFIG0			0x50
#define ICM_4xxxx_FIFO_CONFIG			0x16
#define ICM_4xxxx_FIFO_CONFIG1			0x5F
#define ICM_4xxxx_FIFO_CONFIG2			0x60
#define ICM_4xxxx_FIFO_CONFIG3			0x61
#define ICM_4xxxx_INT_CONFIG0			0x63
#define ICM_4xxxx_INT_CONFIG1			0x64
#define ICM_4xxxx_INT_SOURCE0			0x65
#define ICM_4xxxx_INT_SOURCE1			0x66
#define ICM_4xxxx_INT_SOURCE3			0x68
#define ICM_4xxxx_INT_SOURCE4			0x69
#define ICM_4xxxx_TEMP_DATA1			0x1D
#define ICM_4xxxx_GYRO_DATA_X1			0x25
#define ICM_4xxxx_ACCEL_DATA_X1			0x1F

#define ICM_4xxxx_BANK_SEL				0x76
#define ICM_4xxxx_BANK_SELECT0			0x00
#define ICM_4xxxx_BANK_SELECT1			0x01
#define ICM_4xxxx_BANK_SELECT2			0x02
#define ICM_4xxxx_BANK_SELECT3			0x03
#define ICM_4xxxx_BANK_SELECT4			0x04

#define ICM_4xxxx_GYRO_CONFIG_STATIC3	0x0C
#define ICM_4xxxx_GYRO_CONFIG_STATIC4	0x0D
#define ICM_4xxxx_GYRO_CONFIG_STATIC5	0x0E
#define ICM_4xxxx_ACCEL_CONFIG_STATIC2	0x03
#define ICM_4xxxx_ACCEL_CONFIG_STATIC3	0x04
#define ICM_4xxxx_ACCEL_CONFIG_STATIC4	0x05



#endif // ICM4xxxx_H
