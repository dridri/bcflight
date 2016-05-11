#ifndef SENSOR_H
#define SENSOR_H

#include <list>
#include <functional>
#include <string>
#include <Main.h>
#include <Matrix.h>

class Gyroscope;
class Accelerometer;
class Magnetometer;
class Voltmeter;
class CurrentSensor;

class Sensor
{
public:
	Sensor();
	virtual ~Sensor();

	const std::list< std::string >& names() const;
	const bool calibrated() const;
	Vector4f lastValues() const;

	void setAxisSwap( const int swap[4] );
	void setAxisMatrix( const Matrix& matrix );
	virtual void Calibrate( float dt, bool last_pass = false ) = 0;

	static void AddDevice( Sensor* sensor );
	static void RegisterDevice( int I2Caddr );
	static std::list< Sensor* > Devices();
	static std::list< Gyroscope* > Gyroscopes();
	static std::list< Accelerometer* > Accelerometers();
	static std::list< Magnetometer* > Magnetometers();
	static std::list< Voltmeter* > Voltmeters();
	static std::list< CurrentSensor* > CurrentSensors();
	static Gyroscope* gyroscope( const std::string& name );
	static Accelerometer* accelerometer( const std::string& name );
	static Magnetometer* magnetometer( const std::string& name );
	static Voltmeter* voltmeter( const std::string& name );
	static CurrentSensor* currentSensor( const std::string& name );

protected:
	typedef enum {
		SwapModeNone,
		SwapModeAxis,
		SwapModeMatrix,
	} SwapMode;
	typedef struct {
		int iI2CAddr;
		std::function< Sensor* (void) > fInstanciate;
	} Device;

	std::list< std::string > mNames;
	Vector4f mLastValues;
	bool mCalibrated;
	int mSwapMode;
	int mAxisSwap[4];
	Matrix mAxisMatrix;

	void ApplySwap( Vector3f& v );
	void ApplySwap( Vector4f& v );
	Vector4f SmoothData( const Vector4f& v, bool double_smooth = false );

	static std::list< Device > mKnownDevices; // Contains all the known devices by this software
	static std::list< Sensor* > mDevices; // Contains all the detected devices
	static std::list< Gyroscope* > mGyroscopes; // Contains all the detected gyroscopes
	static std::list< Accelerometer* > mAccelerometers; // ^
	static std::list< Magnetometer* > mMagnetometers; // ^
	static std::list< Voltmeter* > mVoltmeters; // ^
	static std::list< CurrentSensor* > mCurrentSensors; // ^

	static void UpdateDevices();
};

#endif // SENSOR_H
