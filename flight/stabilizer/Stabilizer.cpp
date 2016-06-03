#include <unistd.h>
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
	, mAltitudeHold( false )
	, mRatePID( PID() )
	, mHorizonPID( PID() )
	, mAltitudePID( PID() )
	, mAltitudeControl( 0.0f )
	, mLockState( 0 )
	, mHorizonMultiplier( Vector3f( 15.0f, 15.0f, 1.0f ) )
	, mHorizonOffset( Vector3f() )
{
	mRatePID.setP( std::atof( Board::LoadRegister( "PID:P" ).c_str() ) );
	mRatePID.setI( std::atof( Board::LoadRegister( "PID:I" ).c_str() ) );
	mRatePID.setD( std::atof( Board::LoadRegister( "PID:D" ).c_str() ) );

	mHorizonPID.setP( std::atof( Board::LoadRegister( "PID:Outerloop:P" ).c_str() ) );
	mHorizonPID.setI( std::atof( Board::LoadRegister( "PID:Outerloop:I" ).c_str() ) );
	mHorizonPID.setD( std::atof( Board::LoadRegister( "PID:Outerloop:D" ).c_str() ) );

	mAltitudePID.setP( 0.001 );
	mAltitudePID.setI( 0.010 );
	mAltitudePID.setDeadBand( Vector3f( 0.05f, 0.0f, 0.0f ) );

	float v;
	if ( ( v = main->config()->number( "stabilizer.horizon_angles.x" ) ) > 0.0f ) {
		mHorizonMultiplier.x = v;
	}
	if ( ( v = main->config()->number( "stabilizer.horizon_angles.y" ) ) > 0.0f ) {
		mHorizonMultiplier.y = v;
	}

// 	mHorizonPID.setDeadBand( Vector3f( 0.5f, 0.5f, 0.0f ) );

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


void Stabilizer::setHorizonOffset( const Vector3f& v )
{
	mHorizonOffset = v;
}


Vector3f Stabilizer::horizonOffset() const
{
	return mHorizonOffset;
}


void Stabilizer::setMode( uint32_t mode )
{
	mMode = (Mode)mode;
}


uint32_t Stabilizer::mode() const
{
	return (uint32_t)mMode;
}


void Stabilizer::setAltitudeHold( bool enabled )
{
	mAltitudeHold = enabled;
}


bool Stabilizer::altitudeHold() const
{
	return mAltitudeHold;
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
			control_angles.x = mHorizonMultiplier.x * std::min( std::max( control_angles.x, -1.0f ), 1.0f ) + mHorizonOffset.x;
			control_angles.y = mHorizonMultiplier.y * std::min( std::max( control_angles.y, -1.0f ), 1.0f ) + mHorizonOffset.y;
			mHorizonPID.Process( control_angles, imu->RPY(), dt );
			rate_control = mHorizonPID.state();
			rate_control.z = control_angles.z * mRateFactor; // Bypass heading for now
			break;
		}
	}
	mRatePID.Process( rate_control, imu->rate(), dt );

	float thrust = ctrl->thrust();
	if ( mAltitudeHold ) {
		thrust = thrust * 2.0f - 1.0f;
		if ( std::abs( thrust ) < 0.1f ) {
			thrust = 0.0f;
		} else if ( thrust > 0.0f ) {
			thrust = ( thrust - 0.1f ) * ( 1.0f / 0.9f );
		} else if ( thrust < 0.0f ) {
			thrust = ( thrust + 0.1f ) * ( 1.0f / 0.9f );
		}
		thrust *= 0.01f; // Reduce to 1m/s (assuming controller is sending at 100Hz update rate)
		mAltitudeControl += thrust;
		mAltitudePID.Process( Vector3f( mAltitudeControl, 0.0f, 0.0f ), Vector3f( imu->altitude(), 0.0f, 0.0f ), dt );
		thrust = mAltitudePID.state().x;
	}

	if ( mFrame->Stabilize( mRatePID.state(), thrust ) == false ) {
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
