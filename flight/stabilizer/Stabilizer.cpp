#include <math.h>
#include <Debug.h>
#include <Main.h>
#include <Board.h>
#include <IMU.h>
#include <Controller.h>
#include "Stabilizer.h"

Stabilizer::Stabilizer( Main* main, Frame* frame )
	: mFrame( frame )
	, mMode( Stabilize )
	, mRatePID( PID() )
	, mHorizonPID( PID() )
	, mLockState( 0 )
{
	mRatePID.setP( std::atof( Board::LoadRegister( "PID:P" ).c_str() ) );
	mRatePID.setI( std::atof( Board::LoadRegister( "PID:I" ).c_str() ) );
	mRatePID.setD( std::atof( Board::LoadRegister( "PID:D" ).c_str() ) );

	mHorizonPID.setP( std::atof( Board::LoadRegister( "PID:Outerloop:P" ).c_str() ) );
	mHorizonPID.setI( std::atof( Board::LoadRegister( "PID:Outerloop:I" ).c_str() ) );
	mHorizonPID.setD( std::atof( Board::LoadRegister( "PID:Outerloop:D" ).c_str() ) );

	mRateFactor = main->config()->number( "stabilizer.rate_speed" );
	if ( mRateFactor <= 0.0f ) {
		mRateFactor = 200.0f;
	}
}


void Stabilizer::setP( float p )
{
	mRatePID.setP( p );
	Board::SaveRegister( "PID:P", std::to_string( p ) );
}


void Stabilizer::setI( float i )
{
	mRatePID.setI( i );
	Board::SaveRegister( "PID:I", std::to_string( i ) );
}


void Stabilizer::setD( float d )
{
	mRatePID.setD( d );
	Board::SaveRegister( "PID:D", std::to_string( d ) );
}


Vector3f Stabilizer::getPID() const
{
	return mRatePID.getPID();
}


Vector3f Stabilizer::lastPIDOutput() const
{
	return mHorizonPID.getPID();
}


void Stabilizer::setOuterP( float p )
{
	mHorizonPID.setP( p );
	Board::SaveRegister( "PID:Outerloop:P", std::to_string( p ) );
}


void Stabilizer::setOuterI( float i )
{
	mHorizonPID.setI( i );
	Board::SaveRegister( "PID:Outerloop:I", std::to_string( i ) );
}


void Stabilizer::setOuterD( float d )
{
	mHorizonPID.setD( d );
	Board::SaveRegister( "PID:Outerloop:D", std::to_string( d ) );
}


Vector3f Stabilizer::getOuterPID() const
{
	return mHorizonPID.getPID();
}


Vector3f Stabilizer::lastOuterPIDOutput() const
{
	return mHorizonPID.state();
}


void Stabilizer::setMode( uint32_t mode )
{
	mMode = (Mode)mode;
}


uint32_t Stabilizer::mode() const
{
	return (uint32_t)mMode;
}


void Stabilizer::Reset( const float& yaw )
{
	mRatePID.Reset();
	mHorizonPID.Reset();
}


void Stabilizer::Update( IMU* imu, Controller* ctrl, float dt )
{
	Vector3f rate_control = Vector3f();

	if ( mLockState >= 1 ) {
		mLockState = 2;
		return;
	}

	if ( not ctrl->armed() ) {
		return;
	}

	switch ( mMode ) {
		case Rate : {
			rate_control = ctrl->RPY() * mRateFactor;
			break;
		}
		case ReturnToHome :
		case Follow :
		case Stabilize :
		default : {
			Vector3f control_angles = ctrl->RPY();
			control_angles.x = 30.0f * std::min( std::max( control_angles.x, -1.0f ), 1.0f );
			control_angles.y = 30.0f * std::min( std::max( control_angles.y, -1.0f ), 1.0f );
			mHorizonPID.Process( control_angles, imu->RPY(), dt );
			rate_control = mHorizonPID.state();
			break;
		}
	}

	mRatePID.Process( rate_control, imu->rate(), dt );

	if ( mFrame->Stabilize( mRatePID.state(), ctrl->thrust() ) == false ) {
		Reset( mHorizonPID.state().z );
	}
}


void Stabilizer::CalibrateESCs()
{
	mLockState = 1;
	while ( mLockState != 2 ) {
		usleep( 1000 * 10 );
	}

	mFrame->CalibrateESCs();

	mLockState = 0;
}
