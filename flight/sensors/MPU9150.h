#ifndef MPU9150_H
#define MPU9150_H

#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <I2C.h>
#include <Quaternion.h>


class MPU9150Accel : public Accelerometer
{
public:
	MPU9150Accel( int addr );
	~MPU9150Accel();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	std::string infos();

private:
	I2C* mI2C;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


class MPU9150Gyro : public Gyroscope
{
public:
	MPU9150Gyro( int addr );
	~MPU9150Gyro();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	std::string infos();

private:
	I2C* mI2C;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


class MPU9150Mag : public Magnetometer
{
public:
	MPU9150Mag( int addr );
	~MPU9150Mag();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	std::string infos();

private:
	I2C* mI2C9150;
	I2C* mI2C;
	int mState;
	uint8_t mData[6];
};


class MPU9150 : public Sensor
{
public:
	static int flight_register( Main* main );

	static Sensor* Instanciate();

/*
	uint8_t dmpInitialize();
	bool dmpPacketAvailable();
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
*/
};


#define MPU_9150_I2C_ADDRESS_1          0x69    // Base address of the Drotek board
#define MPU_9150_I2C_ADDRESS_2          0x68    // Base address of the SparkFun board
#define MPU_9150_SMPRT_DIV              0x19    // Gyro sampling rate divider
#define MPU_9150_DEFINE                 0x1A    // Gyro and accel configuration
#define MPU_9150_GYRO_CONFIG            0x1B    // Gyroscope configuration
#define MPU_9150_ACCEL_CONFIG           0x1C    // Accelerometer configuration
#define MPU_9150_FIFO_EN                0x23    // FIFO buffer control
#define MPU_9150_INT_PIN_CFG            0x37    // Bypass enable configuration
#define MPU_9150_INT_ENABLE             0x38    // Interrupt control
#define MPU_9150_ACCEL_XOUT_H           0x3B    // Accel X axis High
#define MPU_9150_ACCEL_XOUT_L           0x3C    // Accel X axis Low
#define MPU_9150_ACCEL_YOUT_H           0x3D    // Accel Y axis High
#define MPU_9150_ACCEL_YOUT_L           0x3E    // Accel Y axis Low
#define MPU_9150_ACCEL_ZOUT_H           0x3F    // Accel Z axis High
#define MPU_9150_ACCEL_ZOUT_L           0x40    // Accel Z axis Low
#define MPU_9150_GYRO_XOUT_H            0x43    // Gyro X axis High
#define MPU_9150_GYRO_XOUT_L            0x44    // Gyro X axis Low
#define MPU_9150_GYRO_YOUT_H            0x45    // Gyro Y axis High
#define MPU_9150_GYRO_YOUT_L            0x46    // Gyro Y axis Low
#define MPU_9150_GYRO_ZOUT_H            0x47    // Gyro Z axis High
#define MPU_9150_GYRO_ZOUT_L            0x48    // Gyro Z axis Low
#define MPU_9150_USER_CTRL              0x6A    // User control
#define MPU_9150_PWR_MGMT_1             0x6B    // Power management 1

#define MPU_9150_I2C_MAGN_ADDRESS       0x0C    // Address of the magnetometer in bypass mode
#define MPU_9150_WIA                    0x00    // Mag Who I Am
#define MPU_9150_AKM_ID                 0x48    // Mag device ID
#define MPU_9150_ST1                    0x02    // Magnetometer status 1
#define MPU_9150_HXL                    0x03    // Mag X axis Low
#define MPU_9150_HXH                    0x04    // Mag X axis High
#define MPU_9150_HYL                    0x05    // Mag Y axis Low
#define MPU_9150_HYH                    0x06    // Mag Y axis High
#define MPU_9150_HZL                    0x07    // Mag Z axis Low
#define MPU_9150_HZH                    0x08    // Mag Z axis High
#define MPU_9150_ST2                    0x09    // Magnetometer status 2
#define MPU_9150_CNTL                   0x0A    // Magnetometer control


#endif // MPU9150_H
