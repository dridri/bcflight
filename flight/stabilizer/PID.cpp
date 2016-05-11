#include "PID.h"

PID::PID()
	: mIntegral( Vector3f() )
	, mLastError( Vector3f() )
	, mkPID( Vector3f() )
	, mState( Vector3f() )
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

	mIntegral += error * dt;
	Vector3f derivative = ( error - mLastError ) / dt;
	Vector3f output = error * mkPID.x + mIntegral * mkPID.y + derivative * mkPID.z;

	mLastError = error;
	mState = output;
}
