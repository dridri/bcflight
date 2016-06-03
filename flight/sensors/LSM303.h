#ifndef LSM303_H
#define LSM303_H

#include <Accelerometer.h>
#include <Magnetometer.h>
#include <I2C.h>


class LSM303Accel : public Accelerometer
{
public:
	LSM303Accel();
	~LSM303Accel();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	static Sensor* Instanciate( Config* config, const std::string& object );

private:
	I2C* mI2C;
	Vector4f mCalibrationAccum;
	Vector3f mOffset;
};


class LSM303Mag : public Magnetometer
{
public:
	LSM303Mag();
	~LSM303Mag();

	void Calibrate( float dt, bool last_pass = false );
	void Read( Vector3f* v, bool raw = false );

	static Sensor* Instanciate( Config* config, const std::string& object );

private:
	I2C* mI2C;
};


class LSM303 : public Sensor
{
public:
	static int flight_register( Main* main );
};

#define ACCEL_PRECISION 12
#define ACCEL_MS2(x) ( (float)x / (float)( 1 << ACCEL_PRECISION ) * 10.0f )

#define LSM303_REGISTER_ACCEL_CTRL_REG1_A 0x20 // 00000111   rw
#define LSM303_REGISTER_ACCEL_CTRL_REG4_A 0x23 // 00000000   rw
#define LSM303_REGISTER_ACCEL_CTRL_REG5_A 0x24
#define LSM303_REGISTER_ACCEL_STATUS_REG_A 0x27
#define LSM303_REGISTER_ACCEL_Oraw_temp_X_L_A   0x28
#define LSM303_REGISTER_MAG_CRB_REG_M     0x01
#define LSM303_REGISTER_MAG_MR_REG_M      0x02
#define LSM303_REGISTER_MAG_Oraw_temp_X_H_M     0x03

#endif // LSM303_H
