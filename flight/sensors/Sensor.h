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
#include <Mutex.h>

class Config;
class Bus;
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
	class Device {
	public:
		int iI2CAddr;//defaultI2Caddr;
		const char* name;
		function< Sensor* ( Config*, const string&, Bus* ) > fInstanciate;
		Device() : iI2CAddr(0), name("") {}
	};

	Sensor();
	virtual ~Sensor();

	const list< string >& names() const;
	const bool calibrated() const;
	Vector4f lastValues() const;

	void setAxisSwap( const int swap[4] );
	void setAxisMatrix( const Matrix& matrix );
	virtual void Calibrate( float dt, bool last_pass = false ) = 0;

	static void AddDevice( Sensor* sensor );
	static void RegisterDevice( int I2Caddr, const string& name = "", Config* config = nullptr, const string& object = "" );
	static void RegisterDevice( const string& name, Config* config, const string& object );
	static const list< Device >& KnownDevices();
	static list< Sensor* > Devices();
	static list< Gyroscope* > Gyroscopes();
	static list< Accelerometer* > Accelerometers();
	static list< Magnetometer* > Magnetometers();
	static list< Altimeter* > Altimeters();
	static list< GPS* > GPSes();
	static list< Voltmeter* > Voltmeters();
	static list< CurrentSensor* > CurrentSensors();
	static Gyroscope* gyroscope( const string& name );
	static Accelerometer* accelerometer( const string& name );
	static Magnetometer* magnetometer( const string& name );
	static Altimeter* altimeter( const string& name );
	static GPS* gps( const string& name );
	static Voltmeter* voltmeter( const string& name );
	static CurrentSensor* currentSensor( const string& name );

	virtual string infos() { return ""; }
	static string infosAll();

protected:
	typedef enum {
		SwapModeNone,
		SwapModeAxis,
		SwapModeMatrix,
	} SwapMode;

	list< string > mNames;
	Vector4f mLastValues;
	bool mCalibrated;
	int mSwapMode;
	int mAxisSwap[4];
	Matrix mAxisMatrix;

	LUA_PROPERTY("bus") Bus* mBus;

	void ApplySwap( Vector3f& v );
	void ApplySwap( Vector4f& v );

	static list< Device > mKnownDevices; // Contains all the known devices by this software
	static list< Sensor* > mDevices; // Contains all the detected devices
	static list< Gyroscope* > mGyroscopes; // Contains all the detected gyroscopes
	static list< Accelerometer* > mAccelerometers; // ^
	static list< Magnetometer* > mMagnetometers; // ^
	static list< Altimeter* > mAltimeters; // ^
	static list< GPS* > mGPSes; // ^
	static list< Voltmeter* > mVoltmeters; // ^
	static list< CurrentSensor* > mCurrentSensors; // ^

	static void UpdateDevices();
};

#endif // SENSOR_H
