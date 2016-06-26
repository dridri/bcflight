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

#ifndef SENSOR_H
#define SENSOR_H

#include <list>
#include <functional>
#include <string>
#include <Main.h>
#include <Matrix.h>

class Config;
class Gyroscope;
class Accelerometer;
class Magnetometer;
class Altimeter;
class GPS;
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
	static void RegisterDevice( const std::string& name, Config* config, const std::string& object );
	static std::list< Sensor* > Devices();
	static std::list< Gyroscope* > Gyroscopes();
	static std::list< Accelerometer* > Accelerometers();
	static std::list< Magnetometer* > Magnetometers();
	static std::list< Altimeter* > Altimeters();
	static std::list< GPS* > GPSes();
	static std::list< Voltmeter* > Voltmeters();
	static std::list< CurrentSensor* > CurrentSensors();
	static Gyroscope* gyroscope( const std::string& name );
	static Accelerometer* accelerometer( const std::string& name );
	static Magnetometer* magnetometer( const std::string& name );
	static Altimeter* altimeter( const std::string& name );
	static GPS* gps( const std::string& name );
	static Voltmeter* voltmeter( const std::string& name );
	static CurrentSensor* currentSensor( const std::string& name );

	virtual std::string infos() { return ""; }
	static std::string infosAll();

protected:
	typedef enum {
		SwapModeNone,
		SwapModeAxis,
		SwapModeMatrix,
	} SwapMode;
/*
	typedef struct {
		int iI2CAddr;
		const char* name;
		std::function< Sensor* ( Config*, const std::string& ) > fInstanciate;
		Device() : iI2CAddr(0), name("") {}
	} Device;
*/
	class Device {
	public:
		int iI2CAddr;
		const char* name;
		std::function< Sensor* ( Config*, const std::string& ) > fInstanciate;
		Device() : iI2CAddr(0), name("") {}
	};

	std::list< std::string > mNames;
	Vector4f mLastValues;
	bool mCalibrated;
	int mSwapMode;
	int mAxisSwap[4];
	Matrix mAxisMatrix;

	void ApplySwap( Vector3f& v );
	void ApplySwap( Vector4f& v );

	static std::list< Device > mKnownDevices; // Contains all the known devices by this software
	static std::list< Sensor* > mDevices; // Contains all the detected devices
	static std::list< Gyroscope* > mGyroscopes; // Contains all the detected gyroscopes
	static std::list< Accelerometer* > mAccelerometers; // ^
	static std::list< Magnetometer* > mMagnetometers; // ^
	static std::list< Altimeter* > mAltimeters; // ^
	static std::list< GPS* > mGPSes; // ^
	static std::list< Voltmeter* > mVoltmeters; // ^
	static std::list< CurrentSensor* > mCurrentSensors; // ^

	static void UpdateDevices();
};

#endif // SENSOR_H
