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
#include <unistd.h>
#include <algorithm>
#include "Main.h"
#include "PowerThread.h"
#include "Config.h"
#include <Sensor.h>
#include <Voltmeter.h>
#include <CurrentSensor.h>
#include <GPIO.h>

PowerThread::PowerThread( Main* main )
	: Thread( "power" )
	, mMain( main )
	, mLowVoltageValue( 9.9 )
	, mLowVoltageBuzzerPin( -1 )
	, mLowVoltageTick( 0 )
	, mLowVoltagePatternCase( 0 )
	, mSaveTicks( Board::GetTicks() )
	, mTicks( Board::GetTicks() )
	, mCellsCount( 0 )
	, mVBat( 0.0f )
	, mCurrentTotal( 0.0f )
	, mCurrentDraw( 0.0f )
	, mBatteryLevel( 0.0f )
	, mBatteryCapacity( 2500.0f )
	, mVoltageSensor{ NONE, nullptr, 0, 0, 0 }
	, mCurrentSensor{ NONE, nullptr, 0, 0, 0 }
{
	mBatteryCapacity = main->config()->Integer( "battery.capacity" );
	if ( mBatteryCapacity == 0.0f ) {
		mBatteryCapacity = 1800.0f;
	}

	if ( 1 ) {
		string /*sensorType*/sensorName = main->config()->String( "battery.voltage.sensor_type" );
		for ( auto it : Sensor::Voltmeters() ) {
			list< string > names = it->names();
			if ( find( names.begin(), names.end(), sensorName ) != names.end() ) {
				mVoltageSensor.type = VOLTAGE;
				mVoltageSensor.sensor = it;
				break;
			}
		}
		if ( mVoltageSensor.sensor == nullptr ) {
			for ( auto it : Sensor::KnownDevices() ) {
				if ( it.name == sensorName ) {
					mVoltageSensor.type = VOLTAGE;
					mVoltageSensor.sensor = it.fInstanciate( main->config(), "battery.voltage", nullptr );
					break;
				}
			}
		}
		if ( mVoltageSensor.sensor == nullptr ) {
			gDebug() << "FATAL ERROR : Unsupported sensor ( " << sensorName << " ) for battery voltage !";
		}
		/*
		if ( sensorType == "Voltmeter" ) {
			mVoltageSensor.type = VOLTAGE;
			mVoltageSensor.sensor = Sensor::voltmeter( main->config()->String( "battery.voltage.device" ) );
		} else {
			gDebug() << "FATAL ERROR : Unsupported sensor type ( " << sensorType << " ) for battery voltage !";
		}
		*/
		mVoltageSensor.channel = main->config()->Integer( "battery.voltage.channel" );
		mVoltageSensor.shift = main->config()->Number( "battery.voltage.shift" );
		mVoltageSensor.multiplier = main->config()->Number( "battery.voltage.multiplier" );
	}
	if ( 1 ) {
		string /*sensorType*/sensorName = main->config()->String( "battery.current.sensor_type" );
		for ( auto it : Sensor::Voltmeters() ) {
			list< string > names = it->names();
			if ( find( names.begin(), names.end(), sensorName ) != names.end() ) {
				mCurrentSensor.type = VOLTAGE;
				mCurrentSensor.sensor = it;
				break;
			}
		}
		for ( auto it : Sensor::CurrentSensors() ) {
			list< string > names = it->names();
			if ( find( names.begin(), names.end(), sensorName ) != names.end() ) {
				mCurrentSensor.type = CURRENT;
				mCurrentSensor.sensor = it;
				break;
			}
		}
		if ( mCurrentSensor.sensor == nullptr ) {
			gDebug() << "FATAL ERROR : Unsupported sensor ( " << sensorName << " ) for battery current !";
		}
		/*
		if ( sensorType == "Voltmeter" ) {
			mCurrentSensor.type = VOLTAGE;
			mCurrentSensor.sensor = Sensor::voltmeter( main->config()->String( "battery.current.device" ) );
		} else if ( sensorType == "CurrentSensor" ) {
			mCurrentSensor.type = CURRENT;
			mCurrentSensor.sensor = Sensor::currentSensor( main->config()->String( "battery.current.device" ) );
		} else {
			gDebug() << "WARNING : Unsupported sensor type ( " << sensorType << " ) for battery current !";
		}
		*/
		mCurrentSensor.channel = main->config()->Integer( "battery.current.channel" );
		mCurrentSensor.shift = main->config()->Number( "battery.current.shift" );
		mCurrentSensor.multiplier = main->config()->Number( "battery.current.multiplier" );
	}

	mLowVoltageValue = main->config()->Number( "battery.low_voltage", 9.9f );
	string low_voltage_trigger = main->config()->String( "battery.low_voltage_trigger.type" );
	if ( low_voltage_trigger == "Buzzer" ) {
		mLowVoltageBuzzerPin = main->config()->Integer( "battery.low_voltage_trigger.pin", -1 );
		if ( mLowVoltageBuzzerPin > 0 ) {
			GPIO::setMode( mLowVoltageBuzzerPin, GPIO::Output );
			GPIO::Write( mLowVoltageBuzzerPin, 0 );
		}
		vector<int> array = main->config()->IntegerArray( "battery.low_voltage_trigger.pattern" );
		for ( uint32_t i = 0; i < array.size(); i++ ) {
			mLowVoltageBuzzerPattern.emplace_back( array[i] * 1000 );
		}
	}

	mLastVBat = atof( Board::LoadRegister( "VBat" ).c_str() );
	mCurrentTotal = atof( Board::LoadRegister( "CurrentTotal" ).c_str() );
	mCellsCount = atof( Board::LoadRegister( "CellsCount" ).c_str() );
}


PowerThread::~PowerThread()
{
}


float PowerThread::VBat() const
{
	return mVBat;
}


float PowerThread::CurrentTotal() const
{
	return mCurrentTotal;
}


float PowerThread::CurrentDraw() const
{
	return mCurrentDraw;
}


float PowerThread::BatteryLevel() const
{
	return mBatteryLevel;
}


void PowerThread::ResetFullBattery( uint32_t capacity_mah )
{
#ifdef SYSTEM_NAME_Linux
	mCapacityMutex.lock();
#endif
	mCellsCount = 0;
	mCurrentTotal = 0.0f;
	if ( capacity_mah != 0 ) {
// 		mBatteryCapacity = (float)capacity_mah;
	}
#ifdef SYSTEM_NAME_Linux
	mCapacityMutex.unlock();
#endif
}


bool PowerThread::run()
{
	float dt = (float)( Board::GetTicks() - mTicks ) / 1000000.0f;
	mTicks = Board::GetTicks();

	if ( abs( dt ) > 2.0 ) {
		gDebug() << "Critical : dt too high !! ( " << dt << " )";
		return true;
	}

	float volt = 0.0f;
	float current = 0.0f;

	if ( mVoltageSensor.sensor ) {
		if ( mVoltageSensor.type == VOLTAGE ) {
			volt = dynamic_cast< Voltmeter* >( mVoltageSensor.sensor )->Read( mVoltageSensor.channel );
		}
		volt = ( volt + mVoltageSensor.shift ) * mVoltageSensor.multiplier;
	}
	if ( mCurrentSensor.sensor ) {
		if ( mCurrentSensor.type == VOLTAGE ) {
			current = static_cast< Voltmeter* >( mCurrentSensor.sensor )->Read( mCurrentSensor.channel );
		} else if ( mCurrentSensor.type == CURRENT ) {
			current = static_cast< CurrentSensor* >( mCurrentSensor.sensor )->Read( mCurrentSensor.channel );
		}
		current = ( current + mCurrentSensor.shift ) * mCurrentSensor.multiplier;
	}

	mVBat = volt;
	mCurrentDraw = current;// / 3600.0f;

#ifdef SYSTEM_NAME_Linux
	mCapacityMutex.lock();
#endif
	if ( mCellsCount == 0 ) {
		if ( mVBat >= 3.365f * 6.0f ) {
			mCellsCount = 6;
		} else if ( mVBat >= 3.365f * 5.0f ) {
			mCellsCount = 5;
		} else if ( mVBat >= 3.365f * 4.0f ) {
			mCellsCount = 4;
		} else if ( mVBat >= 3.365f * 3.0f ) {
			mCellsCount = 3;
		} else if ( mVBat >= 3.365f * 2.0f ) {
			mCellsCount = 2;
		} else if ( mVBat >= 3.365f * 1.0f ) {
			mCellsCount = 1;
		} else {
			mCellsCount = 6;
			gDebug() << "Critical : DANGEROUS BATTERY VOLTAGE ! (" << mVBat << "V)";
		}
	}
#ifdef SYSTEM_NAME_Linux
	mCapacityMutex.unlock();
#endif

	if ( mLowVoltageBuzzerPin > 0 ) {
		if ( mVBat <= mLowVoltageValue * (float)mCellsCount ) {
			if ( mTicks - mLowVoltageTick >= mLowVoltageBuzzerPattern[mLowVoltagePatternCase] ) {
				mLowVoltagePatternCase = ( mLowVoltagePatternCase + 1 ) % mLowVoltageBuzzerPattern.size();
				mLowVoltageTick = mTicks;
			}
			GPIO::Write( mLowVoltageBuzzerPin, ( mLowVoltagePatternCase % 2 ) == 0 );
		} else {
			GPIO::Write( mLowVoltageBuzzerPin, 0 );
		}
	}

#ifdef SYSTEM_NAME_Linux
	mCapacityMutex.lock();
#endif
	mCurrentTotal += current * dt / 3600.0f;
	mBatteryLevel = 1.0f - ( mCurrentTotal * 1000.0f ) / mBatteryCapacity;
	if ( mCurrentSensor.sensor == nullptr ) { // No current sensor
		mBatteryLevel = ( mVBat / (float)mCellsCount - 3.7f ) / ( 4.2f - 3.7f );
	}
	mBatteryLevel = max( 0.0f, min( 1.0f, mBatteryLevel ) );

	if ( Board::GetTicks() - mSaveTicks >= 5 * 1000 * 1000 ) {
		Board::SaveRegister( "VBat", to_string( mVBat ) );
		Board::SaveRegister( "CurrentTotal", to_string( mCurrentTotal ) );
		Board::SaveRegister( "BatteryCapacity", to_string( mBatteryCapacity ) );
		mSaveTicks = Board::GetTicks();
	}

#ifdef SYSTEM_NAME_Linux
	mCapacityMutex.unlock();
#endif

	return true;
}
