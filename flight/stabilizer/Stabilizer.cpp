#include <math.h>
#include <Debug.h>
#include <Board.h>
#include <IMU.h>
#include <Controller.h>
#include "Stabilizer.h"

Stabilizer::Stabilizer( Frame* frame )
	: mFrame( frame )
	, mMode( Stabilize )
	, mSmoothRate( EKFSmoother( EKFSmoother( 3, Vector3f( 1.0f, 1.0f, 1.0f ), Vector3f( 256.0f, 256.0f, 256.0f ) ) ) )
	, mIntegral( Vector3f() )
	, mLastError( Vector3f() )
	, mkPID( Vector3f() )
	, mLastPID( Vector3f() )
	, mTargetRPY( Vector3f() )
	, mOuterIntegral( Vector3f() )
	, mOuterLastError( Vector3f() )
	, mOuterLastOutput( Vector3f() )
	, mOuterkPID( Vector3f() )
	, mLastOuterPID( Vector3f() )
{
	mkPID.x = std::atof( Board::LoadRegister( "PID:P" ).c_str() );
	mkPID.y = std::atof( Board::LoadRegister( "PID:I" ).c_str() );
	mkPID.z = std::atof( Board::LoadRegister( "PID:D" ).c_str() );

	mOuterkPID.x = std::atof( Board::LoadRegister( "PID:Outerloop:P" ).c_str() );
	mOuterkPID.y = std::atof( Board::LoadRegister( "PID:Outerloop:I" ).c_str() );
	mOuterkPID.z = std::atof( Board::LoadRegister( "PID:Outerloop:D" ).c_str() );
}


void Stabilizer::setP( float p )
{
	mkPID.x = p;
	Board::SaveRegister( "PID:P", std::to_string( mkPID.x ) );
}


void Stabilizer::setI( float i )
{
	mkPID.y = i;
	Board::SaveRegister( "PID:I", std::to_string( mkPID.y ) );
}


void Stabilizer::setD( float d )
{
	mkPID.z = d;
	Board::SaveRegister( "PID:D", std::to_string( mkPID.z ) );
}


Vector3f Stabilizer::getPID() const
{
	return mkPID;
}


Vector3f Stabilizer::lastPIDOutput() const
{
	return mLastPID;
}


void Stabilizer::setOuterP( float p )
{
	mOuterkPID.x = p;
	Board::SaveRegister( "PID:Outerloop:P", std::to_string( mOuterkPID.x ) );
}


void Stabilizer::setOuterI( float i )
{
	mOuterkPID.y = i;
	Board::SaveRegister( "PID:Outerloop:I", std::to_string( mOuterkPID.y ) );
}


void Stabilizer::setOuterD( float d )
{
	mOuterkPID.z = d;
	Board::SaveRegister( "PID:Outerloop:D", std::to_string( mOuterkPID.z ) );
}


Vector3f Stabilizer::getOuterPID() const
{
	return mOuterkPID;
}


Vector3f Stabilizer::lastOuterPIDOutput() const
{
	return mOuterLastOutput;
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
	mIntegral = Vector3f();
	mLastError = Vector3f();
	mOuterIntegral = Vector3f();
	mOuterLastError = Vector3f();
	mOuterLastOutput = Vector3f();
	mTargetRPY = Vector3f( 0.0f, 0.0f, yaw );
	mSmoothRate.setState( Vector3f() );
}


void Stabilizer::Update( IMU* imu, Controller* ctrl, float dt )
{
	Vector3f output = Vector3f();

	if ( not ctrl->armed() ) {
// 		mFrame->Disarm();
		return;
	}

	switch ( mMode ) {
		case Rate : {
			output = ProcessRate( imu, ctrl->RPY() * 200.0f, dt );
			break;
		}
		case ReturnToHome :
		case Follow :
		case Stabilize :
		default : {
			Vector3f control_angles = ctrl->RPY();
			control_angles.x = 30.0f * std::min( std::max( control_angles.x, -1.0f ), 1.0f );
			control_angles.y = 30.0f * std::min( std::max( control_angles.y, -1.0f ), 1.0f );
			Vector3f ctrl_rate = ProcessStabilize( imu, control_angles, dt );
			output = ProcessRate( imu, ctrl_rate, dt );
			break;
		}
	}


	if ( ctrl->thrust() == 0.0f ) {
		Reset( imu->RPY().z );
		output = Vector3f();
	}
// 	output = Vector3f(); // TEST
	mFrame->Stabilize( output, ctrl->thrust() );
	mLastPID = output;
}


Vector3f Stabilizer::ProcessRate( IMU* imu, const Vector3f& ctrl_rate, float dt )
{
	mSmoothRate.Predict( dt );
	Vector3f smooth_rate = mSmoothRate.Update( dt, ctrl_rate );

// 	Vector3f error = ctrl_rate - imu->rate();
	Vector3f error = smooth_rate - imu->rate();

// 	error.x = 0.0f;
// 	error.y = 0.0f;
// 	error.z = 0.0f;

	mIntegral += error * dt;
	Vector3f derivative = ( error - mLastError ) / dt;
	Vector3f output;

/*
	PID:P=0.007000
	PID:I=0.010000
	PID:D=0.000700
	PID:Outerloop:P=2.509998
	PID:Outerloop:I=0.000000
	PID:Outerloop:D=0.000000
*/

	output.x = error.x * mkPID.x + mIntegral.x * mkPID.y + derivative.x * mkPID.z;
	output.y = error.y * mkPID.x + mIntegral.y * mkPID.y + derivative.y * mkPID.z;
	output.z = error.z * mkPID.x + mIntegral.z * mkPID.y + derivative.z * mkPID.z;
// 	output.z = error.z * mkPID.x + mIntegral.z * mkPID.y;

// 	output.x = std::max( -50.0f, std::min( 50.0f, output.x ) );
// 	output.y = std::max( -50.0f, std::min( 50.0f, output.y ) );
// 	output.z = std::max( -50.0f, std::min( 50.0f, output.z ) );

	mLastError = error;
	return output;
}


Vector3f Stabilizer::ProcessStabilize( IMU* imu, const Vector3f& ctrl_rpy, float dt )
{
	Vector3f error = ctrl_rpy - imu->RPY();
	CorrectDeadBand( error, Vector3f( 0.1f, 0.1f, 0.5f ) );
	error.x = std::max( -45.0f, std::min( 45.0f, error.x ) );
	error.y = std::max( -45.0f, std::min( 45.0f, error.y ) );
	error.z = std::max( -45.0f, std::min( 45.0f, error.z ) );

	mOuterIntegral += error * dt;
	Vector3f derivative = ( error - mOuterLastError ) / dt;
	Vector3f output = error * mOuterkPID.x + mOuterIntegral * mOuterkPID.y + derivative * mOuterkPID.z;

	mTargetRPY = output;
	mOuterLastOutput = mTargetRPY;
	mOuterLastError = error;
	return output;
}


void Stabilizer::CorrectDeadBand( Vector3f& error, const Vector3f& epsilon )
{
	if ( fabsf( error.x ) < epsilon.x ) {
		error.x = 0.0f;
	}
	if ( fabsf( error.y ) < epsilon.y ) {
		error.y = 0.0f;
	}
	if ( fabsf( error.z ) < epsilon.z ) {
		error.z = 0.0f;
	}
}
