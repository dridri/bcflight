#include "PID.h"

PID::PID()
	: mIntegral( Vector3f() )
	, mLastError( Vector3f() )
	, mkPID( Vector3f() )
	, mState( Vector3f() )
	, mDeadBand( Vector3f() )
{
}


PID::~PID()
{
}


void PID::Reset()
{
	mIntegral = Vector3f();
	mLastError = Vector3f();
	mState = Vector3f();
}


void PID::setP( float p )
{
	mkPID.x = p;
}


void PID::setI( float i )
{
	mkPID.y = i;
}


void PID::setD( float d )
{
	mkPID.z = d;
}


void PID::setDeadBand( const Vector3f& band )
{
	mDeadBand = band;
}


Vector3f PID::getPID() const
{
	return mkPID;
}


Vector3f PID::state() const
{
	return mState;
}


void PID::Process( const Vector3f& command, const Vector3f& measured, float dt )
{
	Vector3f error = command - measured;

	if ( std::abs( error.x ) < mDeadBand.x ) {
		error.x = 0.0f;
	}
	if ( std::abs( error.y ) < mDeadBand.y ) {
		error.y = 0.0f;
	}
	if ( std::abs( error.z ) < mDeadBand.z ) {
		error.z = 0.0f;
	}

	mIntegral += error * dt;
	Vector3f derivative = ( error - mLastError ) / dt;
	Vector3f output = error * mkPID.x + mIntegral * mkPID.y + derivative * mkPID.z;

	mLastError = error;
	mState = output;
}
