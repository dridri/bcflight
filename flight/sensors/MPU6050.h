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

#ifndef MPU6050_H
#define MPU6050_H

#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <I2C.h>
#include <Quaternion.h>
#include <Lua.h>


class MPU6050Accel : public Accelerometer
{
public:
	MPU6050Accel( Bus* bus );
	~MPU6050Accel();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	LuaValue infos();

private:
	Bus* mBus;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


class MPU6050Gyro : public Gyroscope
{
public:
	MPU6050Gyro( Bus* bus );
	~MPU6050Gyro();

	void Calibrate( float dt, bool last_pass = false );
	int Read( Vector3f* v, bool raw = false );

	LuaValue infos();

private:
	Bus* mBus;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


class MPU6050Mag : public Magnetometer
{
public:
	MPU6050Mag( Bus* bus );
	~MPU6050Mag();

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


LUA_CLASS class MPU6050 : public Sensor
{
public:
	LUA_EXPORT MPU6050();

	LUA_PROPERTY() MPU6050Gyro* gyroscope();
	LUA_PROPERTY() MPU6050Accel* accelerometer();
	LUA_PROPERTY() MPU6050Mag* magnetometer();
	void InitChip();

	LUA_PROPERTY("gyroscope_axis_swap") void setGyroscopeAxisSwap( const Vector4i& swap );
	LUA_PROPERTY("accelerometer_axis_swap") void setAccelerometerAxisSwap( const Vector4i& swap );
	LUA_PROPERTY("magnetometer_axis_swap") void setMagnetometerAxisSwap( const Vector4i& swap );

protected:
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
	MPU6050Gyro* mGyroscope;
	MPU6050Accel* mAccelerometer;
	MPU6050Mag* mMagnetometer;
	bool dmpReady;

	void Calibrate( float dt, bool last_pass = false ) {}
	void Read( Vector3f* v, bool raw = false ) {}
};


#define MPU_6050_I2C_ADDRESS_1          0x69    // Base address of the Drotek board
#define MPU_6050_I2C_ADDRESS_2          0x68    // Base address of the SparkFun board
#define MPU_6050_SMPRT_DIV              0x19    // Gyro sampling rate divider
#define MPU_6050_DEFINE                 0x1A    // Gyro and accel configuration
#define MPU_6050_GYRO_CONFIG            0x1B    // Gyroscope configuration
#define MPU_6050_ACCEL_CONFIG           0x1C    // Accelerometer configuration
#define MPU_6050_ACCEL_CONFIG_2         0x1D    // Accelerometer configuration 2
#define MPU_6050_FIFO_EN                0x23    // FIFO buffer control
#define MPU_6050_INT_PIN_CFG            0x37    // Bypass enable configuration
#define MPU_6050_INT_ENABLE             0x38    // Interrupt control
#define MPU_6050_ACCEL_XOUT_H           0x3B    // Accel X axis High
#define MPU_6050_ACCEL_XOUT_L           0x3C    // Accel X axis Low
#define MPU_6050_ACCEL_YOUT_H           0x3D    // Accel Y axis High
#define MPU_6050_ACCEL_YOUT_L           0x3E    // Accel Y axis Low
#define MPU_6050_ACCEL_ZOUT_H           0x3F    // Accel Z axis High
#define MPU_6050_ACCEL_ZOUT_L           0x40    // Accel Z axis Low
#define MPU_6050_GYRO_XOUT_H            0x43    // Gyro X axis High
#define MPU_6050_GYRO_XOUT_L            0x44    // Gyro X axis Low
#define MPU_6050_GYRO_YOUT_H            0x45    // Gyro Y axis High
#define MPU_6050_GYRO_YOUT_L            0x46    // Gyro Y axis Low
#define MPU_6050_GYRO_ZOUT_H            0x47    // Gyro Z axis High
#define MPU_6050_GYRO_ZOUT_L            0x48    // Gyro Z axis Low
#define MPU_6050_USER_CTRL              0x6A    // User control
#define MPU_6050_PWR_MGMT_1             0x6B    // Power management 1
#define MPU_6050_WHO_AM_I               0x75

#define MPU_6050_I2C_MAGN_ADDRESS       0x0C    // Address of the magnetometer in bypass mode
#define MPU_6050_WIA                    0x00    // Mag Who I Am
#define MPU_6050_AKM_ID                 0x48    // Mag device ID
#define MPU_6050_ST1                    0x02    // Magnetometer status 1
#define MPU_6050_HXL                    0x03    // Mag X axis Low
#define MPU_6050_HXH                    0x04    // Mag X axis High
#define MPU_6050_HYL                    0x05    // Mag Y axis Low
#define MPU_6050_HYH                    0x06    // Mag Y axis High
#define MPU_6050_HZL                    0x07    // Mag Z axis Low
#define MPU_6050_HZH                    0x08    // Mag Z axis High
#define MPU_6050_ST2                    0x09    // Magnetometer status 2
#define MPU_6050_CNTL                   0x0A    // Magnetometer control
#define AK8963_ASAX                     0x10    // Fuse ROM x-axis sensitivity adjustment value
#define AK8963_ASAY                     0x11    // Fuse ROM y-axis sensitivity adjustment value
#define AK8963_ASAZ                     0x12    // Fuse ROM z-axis sensitivity adjustment value


#endif // MPU6050_H
