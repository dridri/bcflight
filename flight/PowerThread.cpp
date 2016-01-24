#include <unistd.h>
#include "Main.h"
#include "PowerThread.h"
#include <Sensor.h>
#include <Voltmeter.h>
#include <CurrentSensor.h>

PowerThread::PowerThread( Main* main )
	: Thread( "power" )
	, mTicks( Board::GetTicks() )
	, mMain( main )
	, mVBat( 0.0f )
	, mCurrentTotal( 0.0f )
	, mCurrentDraw( 0.0f )
	, mBatteryCapacity( 2500.0f )
{
	mLastVBat = std::atof( Board::LoadRegister( "VBat" ).c_str() );
	mCurrentTotal = std::atof( Board::LoadRegister( "CurrentTotal" ).c_str() );
	mBatteryCapacity = std::atof( Board::LoadRegister( "BatteryCapacity" ).c_str() );
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


void PowerThread::ResetFullBattery( uint32_t capacity_mah )
{
	mCurrentTotal = 0.0f;
	mBatteryCapacity = (float)capacity_mah;
	Board::SaveRegister( "VBat", std::to_string( mVBat ) );
	Board::SaveRegister( "CurrentTotal", std::to_string( mCurrentTotal ) );
	Board::SaveRegister( "BatteryCapacity", std::to_string( mBatteryCapacity ) );
}


bool PowerThread::run()
{
	float dt = (float)( Board::GetTicks() - mTicks ) / 1000000.0f;
	mTicks = Board::GetTicks();

	float volt = 0.0f;
	float current = 0.0f;

	if ( Sensor::Voltmeters().size() > 0 ) {
		// TODO : dissociate VBat from others voltmeters
		volt = Sensor::Voltmeters().front()->Read() * 3.0f;
	}
	if ( Sensor::CurrentSensors().size() > 0 ) {
		// TODO : dissociate VCurrent from others current sensors
		current = Sensor::CurrentSensors().front()->Read();
	}

	mVBat = volt;
	mCurrentTotal += current * dt / 3600.0f;
	mCurrentDraw = current / 3600.0f;

	if ( mTicks % ( 10 * 1000 * 1000 ) == 0 ) {
		Board::SaveRegister( "VBat", std::to_string( mVBat ) );
		Board::SaveRegister( "CurrentTotal", std::to_string( mCurrentTotal ) );
		Board::SaveRegister( "BatteryCapacity", std::to_string( mBatteryCapacity ) );
	}

	usleep( 1000 * 50 );
	return true;
}
