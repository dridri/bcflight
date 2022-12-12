#include "StabilizerProxy.h"
#include "Slave.h"
#include "Thread.h"


StabilizerProxy::StabilizerProxy( Bus* bus )
	: Stabilizer()
	, mBus( bus )
{
}


StabilizerProxy::~StabilizerProxy()
{
}


void StabilizerProxy::setRollP( float p )
{
	mBus->WriteFloat( Slave::STAB_ROLL_P, p );
	Stabilizer::setRollP( p );
}


void StabilizerProxy::setRollI( float i )
{
	mBus->WriteFloat( Slave::STAB_ROLL_I, i );
	Stabilizer::setRollI( i );
}


void StabilizerProxy::setRollD( float d )
{
	mBus->WriteFloat( Slave::STAB_ROLL_D, d );
	Stabilizer::setRollD( d );
}


void StabilizerProxy::setPitchP( float p )
{
	mBus->WriteFloat( Slave::STAB_PITCH_P, p );
	Stabilizer::setPitchP( p );
}


void StabilizerProxy::setPitchI( float i )
{
	mBus->WriteFloat( Slave::STAB_PITCH_I, i );
	Stabilizer::setPitchI( i );
}


void StabilizerProxy::setPitchD( float d )
{
	mBus->WriteFloat( Slave::STAB_PITCH_D, d );
	Stabilizer::setPitchD( d );
}


void StabilizerProxy::setYawP( float p )
{
	mBus->WriteFloat( Slave::STAB_YAW_P, p );
	Stabilizer::setYawP( p );
}


void StabilizerProxy::setYawI( float i )
{
	mBus->WriteFloat( Slave::STAB_YAW_I, i );
	Stabilizer::setYawI( i );
}


void StabilizerProxy::setYawD( float d )
{
	mBus->WriteFloat( Slave::STAB_YAW_D, d );
	Stabilizer::setYawD( d );
}


Vector3f StabilizerProxy::lastPIDOutput() const
{
	// TODO
}


void StabilizerProxy::setOuterP( float p )
{
	mBus->WriteFloat( Slave::STAB_OUTER_P, p );
	Stabilizer::setOuterP( p );
}


void StabilizerProxy::setOuterI( float i )
{
	mBus->WriteFloat( Slave::STAB_OUTER_I, i );
	Stabilizer::setOuterI( i );
}


void StabilizerProxy::setOuterD( float d )
{
	mBus->WriteFloat( Slave::STAB_OUTER_D, d );
	Stabilizer::setOuterD( d );
}


Vector3f StabilizerProxy::lastOuterPIDOutput() const
{
	// TODO
}


void StabilizerProxy::setHorizonOffset( const Vector3f& v )
{
	mBus->WriteVector3f( Slave::STAB_HORIZ_OFFSET, v );
	Stabilizer::setHorizonOffset( v );
}


void StabilizerProxy::setMode( uint32_t mode )
{
	mBus->Write8( Slave::STAB_MODE, mode );
	Stabilizer::setMode( mode );
}


void StabilizerProxy::setAltitudeHold( bool enabled )
{
	mBus->Write8( Slave::STAB_ALTI_HOLD, enabled );
	Stabilizer::setAltitudeHold( enabled );
}


void StabilizerProxy::Arm()
{
	mBus->Write8( Slave::STAB_ARM, 1 );
	Stabilizer::Arm();
}


void StabilizerProxy::Disarm()
{
	// Send it twice, just in case
	mBus->Write8( Slave::STAB_ARM, 0 );
	Thread::msleep( 5 );
	mBus->Write8( Slave::STAB_ARM, 0 );
	Stabilizer::Disarm();
}


void StabilizerProxy::setRoll( float value )
{
	mBus->WriteFloat( Slave::STAB_ROLL, value );
	Stabilizer::setRoll( value );
}


void StabilizerProxy::setPitch( float value )
{
	mBus->WriteFloat( Slave::STAB_PITCH, value );
	Stabilizer::setPitch( value );
}


void StabilizerProxy::setYaw( float value )
{
	mBus->WriteFloat( Slave::STAB_YAW, value );
	Stabilizer::setYaw( value );
}


void StabilizerProxy::setThrust( float value )
{
	mBus->WriteFloat( Slave::STAB_THRUST, value );
	Stabilizer::setThrust( value );
}


void StabilizerProxy::CalibrateESCs()
{
	mBus->Write8( Slave::STAB_CALIBRATE_ESCS, 1 );
}


void StabilizerProxy::MotorTest( uint32_t id )
{
	// TODO
}


void StabilizerProxy::Reset( const float& yaw )
{
	mBus->Write8( Slave::STAB_RESET, 1 );
}


void StabilizerProxy::Update( IMU* imu, Controller* ctrl, float dt )
{
	(void)imu;
	(void)ctrl;
	(void)dt;
}
