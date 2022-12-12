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

#include "Sensor.h"
#include "Gyroscope.h"
#include "Accelerometer.h"
#include "Magnetometer.h"
#include "GPS.h"
#include "Altimeter.h"
#include "Voltmeter.h"
#include "CurrentSensor.h"
#include <Matrix.h>
#include "I2C.h"
#include "SPI.h"
#include <Config.h>

list< Sensor::Device > Sensor::mKnownDevices;
list< Sensor* > Sensor::mDevices;
list< Gyroscope* > Sensor::mGyroscopes;
list< Accelerometer* > Sensor::mAccelerometers;
list< Magnetometer* > Sensor::mMagnetometers;
list< Altimeter* > Sensor::mAltimeters;
list< GPS* > Sensor::mGPSes;
list< Voltmeter* > Sensor::mVoltmeters;
list< CurrentSensor* > Sensor::mCurrentSensors;

Sensor::Sensor()
	: mNames( std::list<string>() )
	, mCalibrated( false )
	, mSwapMode( SwapModeNone )
	, mAxisSwap{ 0, 0, 0, 0 }
	, mAxisMatrix( Matrix( 4, 4 ) )
{
	mDevices.push_back( this );
	UpdateDevices();
}


Sensor::~Sensor()
{
}


const list< string >& Sensor::names() const
{
	return mNames;
}


const bool Sensor::calibrated() const
{
	return mCalibrated;
}


Vector4f Sensor::lastValues() const
{
	return mLastValues;
}


void Sensor::setAxisSwap( const int swap[4] )
{
	memcpy( mAxisSwap, swap, sizeof(int)*4 );
	if ( mSwapMode == SwapModeNone ) {
		mSwapMode = SwapModeAxis;
	}
}


void Sensor::setAxisMatrix( const Matrix& matrix )
{
	mAxisMatrix = matrix;
	mSwapMode = SwapModeMatrix;
}


void Sensor::ApplySwap( Vector3f& v )
{
	Vector3f tmp = Vector3f( v.x, v.y, v.z );
	if ( mSwapMode == SwapModeAxis ) {
		v[0] = tmp[ abs( mAxisSwap[0] ) - 1 ];
		v[1] = tmp[ abs( mAxisSwap[1] ) - 1 ];
		v[2] = tmp[ abs( mAxisSwap[2] ) - 1 ];
		if ( mAxisSwap[0] < 0 ) {
			v[0] = -v[0];
		}
		if ( mAxisSwap[1] < 0 ) {
			v[1] = -v[1];
		}
		if ( mAxisSwap[2] < 0 ) {
			v[2] = -v[2];
		}
	} else if ( mSwapMode == SwapModeMatrix ) {
		v = mAxisMatrix * tmp;
	}
}


void Sensor::ApplySwap( Vector4f& v )
{
	Vector4f tmp = Vector4f( v.x, v.y, v.z, v.w );
	if ( mSwapMode == SwapModeAxis ) {
		v[0] = tmp[ abs( mAxisSwap[0] ) - 1 ];
		v[1] = tmp[ abs( mAxisSwap[1] ) - 1 ];
		v[2] = tmp[ abs( mAxisSwap[2] ) - 1 ];
		v[3] = tmp[ abs( mAxisSwap[3] ) - 1 ];
		if ( mAxisSwap[0] < 0 ) {
			v[0] = -v[0];
		}
		if ( mAxisSwap[1] < 0 ) {
			v[1] = -v[1];
		}
		if ( mAxisSwap[2] < 0 ) {
			v[2] = -v[2];
		}
		if ( mAxisSwap[3] < 0 ) {
			v[3] = -v[3];
		}
	} else if ( mSwapMode == SwapModeMatrix ) {
		v = mAxisMatrix * tmp;
	}
}


void Sensor::AddDevice( Sensor* sensor )
{
	mDevices.push_back( sensor );
	UpdateDevices();
}


void Sensor::RegisterDevice( int I2Caddr, const string& name, Config* config, const string& object )
{
	for ( Device d : mKnownDevices ) {
		if ( d.iI2CAddr == I2Caddr and ( name == "" or !strcmp( d.name, name.c_str() ) ) ) {
			Sensor* dev = d.fInstanciate( config, object, new I2C(d.iI2CAddr) );
			if ( dev ) {
				mDevices.push_back( dev );
				UpdateDevices();
				break;
			}
		}
	}
}


void Sensor::RegisterDevice( const string& name, Config* config, const string& object )
{
	fDebug( name, config, object );
	for ( Device d : mKnownDevices ) {
		if ( /*d.iI2CAddr == 0 and */!strcmp( d.name, name.c_str() ) ) {
			const string dev = config->String( object + ".device" );
			Bus* bus = nullptr;
			if ( dev.find("spi") != string::npos or dev.find("SPI") != string::npos or dev.find("Spi") != string::npos ) {
				bus = new SPI( dev, config->Integer( object + ".speed", 500000 ) );
			}
			if ( bus ) {
				Sensor* dev = d.fInstanciate( config, object, bus );
				if ( dev ) {
					mDevices.push_back( dev );
					UpdateDevices();
					break;
				}
			}
		}
	}
}


const list< Sensor::Device >& Sensor::KnownDevices()
{
	return mKnownDevices;
}


list< Sensor* > Sensor::Devices()
{
	return mDevices;
}


list< Gyroscope* > Sensor::Gyroscopes()
{
	return mGyroscopes;
}


list< Accelerometer* > Sensor::Accelerometers()
{
	return mAccelerometers;
}


list< Magnetometer* > Sensor::Magnetometers()
{
	return mMagnetometers;
}


list< Altimeter* > Sensor::Altimeters()
{
	return mAltimeters;
}


list< GPS* > Sensor::GPSes()
{
	return mGPSes;
}


list< Voltmeter* > Sensor::Voltmeters()
{
	return mVoltmeters;
}


list< CurrentSensor* > Sensor::CurrentSensors()
{
	return mCurrentSensors;
}


Gyroscope* Sensor::gyroscope( const string& name )
{
	for ( auto it = mGyroscopes.begin(); it != mGyroscopes.end(); it++ ) {
		Gyroscope* g = (*it);
		for ( auto it2 = g->names().begin(); it2 != g->names().end(); it2++ ) {
			if ( (*it2) == name ) {
				return g;
			}
		}
	}
	return nullptr;
}


Accelerometer* Sensor::accelerometer( const string& name )
{
	for ( auto it = mAccelerometers.begin(); it != mAccelerometers.end(); it++ ) {
		Accelerometer* a = (*it);
		for ( auto it2 = a->names().begin(); it2 != a->names().end(); it2++ ) {
			if ( (*it2) == name ) {
				return a;
			}
		}
	}
	return nullptr;
}


Magnetometer* Sensor::magnetometer( const string& name )
{
	for ( auto it = mMagnetometers.begin(); it != mMagnetometers.end(); it++ ) {
		Magnetometer* m = (*it);
		for ( auto it2 = m->names().begin(); it2 != m->names().end(); it2++ ) {
			if ( (*it2) == name ) {
				return m;
			}
		}
	}
	return nullptr;
}


GPS* Sensor::gps( const string& name )
{
	for ( auto it = mGPSes.begin(); it != mGPSes.end(); it++ ) {
		GPS* g = (*it);
		for ( auto it2 = g->names().begin(); it2 != g->names().end(); it2++ ) {
			if ( (*it2) == name ) {
				return g;
			}
		}
	}
	return nullptr;
}


Altimeter* Sensor::altimeter( const string& name )
{
	for ( auto it = mAltimeters.begin(); it != mAltimeters.end(); it++ ) {
		Altimeter* a = (*it);
		for ( auto it2 = a->names().begin(); it2 != a->names().end(); it2++ ) {
			if ( (*it2) == name ) {
				return a;
			}
		}
	}
	return nullptr;
}


Voltmeter* Sensor::voltmeter( const string& name )
{
	for ( auto it = mVoltmeters.begin(); it != mVoltmeters.end(); it++ ) {
		Voltmeter* v = (*it);
		for ( auto it2 = v->names().begin(); it2 != v->names().end(); it2++ ) {
			if ( (*it2) == name ) {
				return v;
			}
		}
	}
	return nullptr;
}


CurrentSensor* Sensor::currentSensor( const string& name )
{
	for ( auto it = mCurrentSensors.begin(); it != mCurrentSensors.end(); it++ ) {
		CurrentSensor* c = (*it);
		for ( auto it2 = c->names().begin(); it2 != c->names().end(); it2++ ) {
			if ( (*it2) == name ) {
				return c;
			}
		}
	}
	return nullptr;
}


void Sensor::UpdateDevices()
{
	mGyroscopes.clear();
	mAccelerometers.clear();
	mMagnetometers.clear();
	mAltimeters.clear();
	mGPSes.clear();
	mVoltmeters.clear();
	mCurrentSensors.clear();

	for ( Sensor* s : mDevices ) {
		Gyroscope* g = dynamic_cast< Gyroscope* >( s );
		if ( g != nullptr ) {
			mGyroscopes.push_back( g );
		}
	}

	for ( Sensor* s : mDevices ) {
		Accelerometer* a = dynamic_cast< Accelerometer* >( s );
		if ( a != nullptr ) {
			mAccelerometers.push_back( a );
		}
	}

	for ( Sensor* s : mDevices ) {
		Magnetometer* m = dynamic_cast< Magnetometer* >( s );
		if ( m != nullptr ) {
			mMagnetometers.push_back( m );
		}
	}

	for ( Sensor* s : mDevices ) {
		GPS* g = dynamic_cast< GPS* >( s );
		if ( g != nullptr ) {
			mGPSes.push_back( g );
		}
	}

	for ( Sensor* s : mDevices ) {
		Altimeter* a = dynamic_cast< Altimeter* >( s );
		if ( a != nullptr ) {
			mAltimeters.push_back( a );
		}
	}

	for ( Sensor* s : mDevices ) {
		Voltmeter* v = dynamic_cast< Voltmeter* >( s );
		if ( v != nullptr ) {
			mVoltmeters.push_back( v );
		}
	}

	for ( Sensor* s : mDevices ) {
		CurrentSensor* c = dynamic_cast< CurrentSensor* >( s );
		if ( c != nullptr ) {
			mCurrentSensors.push_back( c );
		}
	}
}


string Sensor::infosAll()
{
	stringstream ss;
	ss << "Sensors = {\n";

	ss << "\tGyroscopes = {\n";
	for ( Gyroscope* gyro : mGyroscopes ) {
		ss << "\t\t" << gyro->names().front() << " = { " << gyro->infos() << " },\n";
	}
	ss << "\t},\n";

	ss << "\tAccelerometers = {\n";
	for ( Accelerometer* accel : mAccelerometers ) {
		ss << "\t\t" << accel->names().front() << " = { " << accel->infos() << " },\n";
	}
	ss << "\t},\n";

	ss << "\tMagnetometers = {\n";
	for ( Magnetometer* magn : mMagnetometers ) {
		ss << "\t\t" << magn->names().front() << " = { " << magn->infos() << " },\n";
	}
	ss << "\t},\n";

	ss << "\tVoltmeters = {\n";
	for ( Voltmeter* v : mVoltmeters ) {
		ss << "\t\t" << v->names().front() << " = { " << v->infos() << " },\n";
	}
	ss << "\t},\n";

	ss << "\tCurrentSensors = {\n";
	for ( CurrentSensor* c : mCurrentSensors ) {
		ss << "\t\t" << c->names().front() << " = { " << c->infos() << " },\n";
	}
	ss << "\t},\n";

	ss << "}\n";
	return ss.str();
}
