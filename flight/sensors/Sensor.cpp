#include "Sensor.h"
#include "Gyroscope.h"
#include "Accelerometer.h"
#include "Magnetometer.h"
#include "Voltmeter.h"
#include "CurrentSensor.h"
#include <Matrix.h>

std::list< Sensor::Device > Sensor::mKnownDevices;
std::list< Sensor* > Sensor::mDevices;
std::list< Gyroscope* > Sensor::mGyroscopes;
std::list< Accelerometer* > Sensor::mAccelerometers;
std::list< Magnetometer* > Sensor::mMagnetometers;
std::list< Voltmeter* > Sensor::mVoltmeters;
std::list< CurrentSensor* > Sensor::mCurrentSensors;

Sensor::Sensor()
	: mCalibrated( false )
	, mSwapMode( SwapModeNone )
	, mAxisSwap{ 0, 0, 0, 0 }
	, mAxisMatrix( Matrix( 4, 4 ) )
{
}


Sensor::~Sensor()
{
}


const std::list< std::string >& Sensor::names() const
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


Vector4f Sensor::SmoothData( const Vector4f& v, bool double_smooth )
{
	return v;
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
	Vector3f tmp = v;
	if ( mSwapMode == SwapModeAxis ) {
		v[0] = tmp[ std::abs( mAxisSwap[0] ) ] * (float)( mAxisSwap[0] / std::abs( mAxisSwap[0] ) );
		v[1] = tmp[ std::abs( mAxisSwap[1] ) ] * (float)( mAxisSwap[1] / std::abs( mAxisSwap[1] ) );
		v[2] = tmp[ std::abs( mAxisSwap[2] ) ] * (float)( mAxisSwap[2] / std::abs( mAxisSwap[2] ) );
	} else if ( mSwapMode == SwapModeMatrix ) {
		v = mAxisMatrix * tmp;
	}
}


void Sensor::ApplySwap( Vector4f& v )
{
	Vector4f tmp = v;
	if ( mSwapMode == SwapModeAxis ) {
		v[0] = tmp[ std::abs( mAxisSwap[0] ) ] * (float)( mAxisSwap[0] / std::abs( mAxisSwap[0] ) );
		v[1] = tmp[ std::abs( mAxisSwap[1] ) ] * (float)( mAxisSwap[1] / std::abs( mAxisSwap[1] ) );
		v[2] = tmp[ std::abs( mAxisSwap[2] ) ] * (float)( mAxisSwap[2] / std::abs( mAxisSwap[2] ) );
		v[3] = tmp[ std::abs( mAxisSwap[3] ) ] * (float)( mAxisSwap[3] / std::abs( mAxisSwap[3] ) );
	} else if ( mSwapMode == SwapModeMatrix ) {
		v = mAxisMatrix * tmp;
	}
}


void Sensor::AddDevice( Sensor* sensor )
{
	mDevices.push_back( sensor );
	UpdateDevices();
}


void Sensor::RegisterDevice( int I2Caddr )
{
	for ( Device d : mKnownDevices ) {
		if ( d.iI2CAddr == I2Caddr ) {
			Sensor* dev = d.fInstanciate();
			if ( dev ) {
				mDevices.push_back( dev );
			}
			UpdateDevices();
		}
	}
}


std::list< Sensor* > Sensor::Devices()
{
	return mDevices;
}


std::list< Gyroscope* > Sensor::Gyroscopes()
{
	return mGyroscopes;
}


std::list< Accelerometer* > Sensor::Accelerometers()
{
	return mAccelerometers;
}


std::list< Magnetometer* > Sensor::Magnetometers()
{
	return mMagnetometers;
}


std::list< Voltmeter* > Sensor::Voltmeters()
{
	return mVoltmeters;
}


std::list< CurrentSensor* > Sensor::CurrentSensors()
{
	return mCurrentSensors;
}


Voltmeter* Sensor::voltmeter( const std::string& name )
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


CurrentSensor* Sensor::currentSensor( const std::string& name )
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
