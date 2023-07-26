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
#include <math.h>
#include <Debug.h>
#include <Main.h>
#include <Board.h>
#include <IMU.h>
#include <Controller.h>
#include "Config.h"
#include "Stabilizer.h"

Stabilizer::Stabilizer()
	: mMain( Main::instance() )
	, mFrame( nullptr )
	, mMode( Rate )
	, mAltitudeHold( false )
	, mRateRollPID( PID<float>() )
	, mRatePitchPID( PID<float>() )
	, mRateYawPID( PID<float>() )
	, mHorizonPID( PID<Vector3f>() )
	, mAltitudePID( PID<float>() )
	, mAltitudeControl( 0.0f )
	, mArmed( false )
	, mLockState( 0 )
	, mFilteredRPYDerivative( Vector3f() )
	, mHorizonMultiplier( Vector3f( 15.0f, 15.0f, 1.0f ) )
	, mHorizonOffset( Vector3f() )
	, mHorizonMaxRate( Vector3f( 300.0f, 300.0f, 300.0f ) )
{
	fDebug(this);
/*
	mRateRollPID.setP( Board::LoadRegisterFloat( "PID:Roll:P", main->config()->Number( "stabilizer.pid_roll.p" ) ) );
	mRateRollPID.setI( Board::LoadRegisterFloat( "PID:Roll:I", main->config()->Number( "stabilizer.pid_roll.i" ) ) );
	mRateRollPID.setD( Board::LoadRegisterFloat( "PID:Roll:D", main->config()->Number( "stabilizer.pid_roll.d" ) ) );
	mRatePitchPID.setP( Board::LoadRegisterFloat( "PID:Pitch:P", main->config()->Number( "stabilizer.pid_pitch.p" ) ) );
	mRatePitchPID.setI( Board::LoadRegisterFloat( "PID:Pitch:I", main->config()->Number( "stabilizer.pid_pitch.i" ) ) );
	mRatePitchPID.setD( Board::LoadRegisterFloat( "PID:Pitch:D", main->config()->Number( "stabilizer.pid_pitch.d" ) ) );
	mRateYawPID.setP( Board::LoadRegisterFloat( "PID:Yaw:P", main->config()->Number( "stabilizer.pid_yaw.p" ) ) );
	mRateYawPID.setI( Board::LoadRegisterFloat( "PID:Yaw:I", main->config()->Number( "stabilizer.pid_yaw.i" ) ) );
	mRateYawPID.setD( Board::LoadRegisterFloat( "PID:Yaw:D", main->config()->Number( "stabilizer.pid_yaw.d" ) ) );

	mHorizonPID.setP( Board::LoadRegisterFloat( "PID:Outerloop:P", main->config()->Number( "stabilizer.pid_horizon.p" ) ) );
	mHorizonPID.setI( Board::LoadRegisterFloat( "PID:Outerloop:I", main->config()->Number( "stabilizer.pid_horizon.i" ) ) );
	mHorizonPID.setD( Board::LoadRegisterFloat( "PID:Outerloop:D", main->config()->Number( "stabilizer.pid_horizon.d" ) ) );
*/

	mAltitudePID.setP( 0.001 );
	mAltitudePID.setI( 0.010 );
	mAltitudePID.setDeadBand( 0.05f );

	mDerivativeFilter = new PT1<Vector3f>( Vector3f(80, 80, 80) );

// 	mHorizonPID.setDeadBand( Vector3f( 0.5f, 0.5f, 0.0f ) );
}


Stabilizer::~Stabilizer()
{
}


Frame* Stabilizer::frame() const
{
	return mFrame;
}


void Stabilizer::setFrame( Frame* frame )
{
	mFrame = frame;
}


void Stabilizer::setRollP( float p )
{
	mRateRollPID.setP( p );
	Board::SaveRegister( "PID:Roll:P", to_string( p ) );
}


void Stabilizer::setRollI( float i )
{
	mRateRollPID.setI( i );
	Board::SaveRegister( "PID:Roll:I", to_string( i ) );
}


void Stabilizer::setRollD( float d )
{
	mRateRollPID.setD( d );
	Board::SaveRegister( "PID:Roll:D", to_string( d ) );
}


Vector3f Stabilizer::getRollPID() const
{
	return mRateRollPID.getPID();
}


void Stabilizer::setPitchP( float p )
{
	mRatePitchPID.setP( p );
	Board::SaveRegister( "PID:Pitch:P", to_string( p ) );
}


void Stabilizer::setPitchI( float i )
{
	mRatePitchPID.setI( i );
	Board::SaveRegister( "PID:Pitch:I", to_string( i ) );
}


void Stabilizer::setPitchD( float d )
{
	mRatePitchPID.setD( d );
	Board::SaveRegister( "PID:Pitch:D", to_string( d ) );
}


Vector3f Stabilizer::getPitchPID() const
{
	return mRatePitchPID.getPID();
}


void Stabilizer::setYawP( float p )
{
	mRateYawPID.setP( p );
	Board::SaveRegister( "PID:Yaw:P", to_string( p ) );
}


void Stabilizer::setYawI( float i )
{
	mRateYawPID.setI( i );
	Board::SaveRegister( "PID:Yaw:I", to_string( i ) );
}


void Stabilizer::setYawD( float d )
{
	mRateYawPID.setD( d );
	Board::SaveRegister( "PID:Yaw:D", to_string( d ) );
}


Vector3f Stabilizer::getYawPID() const
{
	return mRateYawPID.getPID();
}


Vector3f Stabilizer::lastPIDOutput() const
{
	return Vector3f( mRateRollPID.state(), mRatePitchPID.state(), mRateYawPID.state() );
}


void Stabilizer::setOuterP( float p )
{
	mHorizonPID.setP( p );
	Board::SaveRegister( "PID:Outerloop:P", to_string( p ) );
}


void Stabilizer::setOuterI( float i )
{
	mHorizonPID.setI( i );
	Board::SaveRegister( "PID:Outerloop:I", to_string( i ) );
}


void Stabilizer::setOuterD( float d )
{
	mHorizonPID.setD( d );
	Board::SaveRegister( "PID:Outerloop:D", to_string( d ) );
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


void Stabilizer::Arm()
{
	mArmed = true;
}


void Stabilizer::Disarm()
{
	mArmed = false;
}


bool Stabilizer::armed() const
{
	return mArmed;
}


float Stabilizer::thrust() const
{
	return mThrust;
}


const Vector3f& Stabilizer::RPY() const
{
	return mRPY;
}


const Vector3f& Stabilizer::filteredRPYDerivative() const
{
	return mFilteredRPYDerivative;
}


void Stabilizer::Reset( const float& yaw )
{
	mRateRollPID.Reset();
	mRatePitchPID.Reset();
	mRateYawPID.Reset();
	mHorizonPID.Reset();
	mRPY.x = 0.0f;
	mRPY.y = 0.0f;
	mRPY.z = 0.0f;
	mThrust = 0.0f;
}


void Stabilizer::setRoll( float value )
{
	mRPY.x = value;
}


void Stabilizer::setPitch( float value )
{
	mRPY.y = value;
}


void Stabilizer::setYaw( float value )
{
	mRPY.z = value;
}


void Stabilizer::setThrust( float value )
{
	mThrust = value;
}


void Stabilizer::Update( IMU* imu, Controller* ctrl, float dt )
{
	Vector3f rate_control = Vector3f();

	if ( mLockState >= 1 ) {
		mLockState = 2;
		return;
	}

	Vector3f rates = imu->rate();

	mFilteredRPYDerivative = mDerivativeFilter->filter( rates, dt );

	if ( not mArmed ) {
		return;
	}

	// We didn't take off yet (or really close to ground just after take-off), so we bypass stabilization, and just return
	if ( mFrame->airMode() == false and mThrust <= 0.15f ) {
		mFrame->Stabilize( Vector3f( 0.0f, 0.0f, 0.0f ), mThrust );
		return;
	}

	switch ( mMode ) {
		case Rate : {
			rate_control = mRPY * mRateFactor;
			break;
		}
		case ReturnToHome :
		case Follow :
		case Stabilize :
		default : {
			Vector3f control_angles = mRPY;
			control_angles.x = mHorizonMultiplier.x * min( max( control_angles.x, -1.0f ), 1.0f ) + mHorizonOffset.x;
			control_angles.y = mHorizonMultiplier.y * min( max( control_angles.y, -1.0f ), 1.0f ) + mHorizonOffset.y;
			// TODO : when user-input is 0, set control_angles by using imu->velocity() to compensate position drifting, if enabled
			mHorizonPID.Process( control_angles, imu->RPY(), dt );
			rate_control = mHorizonPID.state();
			rate_control.x = max( -mHorizonMaxRate.x, min( mHorizonMaxRate.x, rate_control.x ) );
			rate_control.y = max( -mHorizonMaxRate.y, min( mHorizonMaxRate.y, rate_control.y ) );
			rate_control.z = control_angles.z * mRateFactor; // TEST : Bypass heading for now
			break;
		}
	}

	float deltaR = rate_control.x - rates.x;
	float deltaP = rate_control.y - rates.y;
	float deltaY = rate_control.z - rates.z;
	mRateRollPID.Process( deltaR, deltaR, rate_control.x - mFilteredRPYDerivative.x, dt );
	mRatePitchPID.Process( deltaP, deltaP, rate_control.y - mFilteredRPYDerivative.y, dt );
	mRateYawPID.Process( deltaY, deltaY, rate_control.z - mFilteredRPYDerivative.z, dt );

	float thrust = mThrust;
	if ( mAltitudeHold ) {
		thrust = thrust * 2.0f - 1.0f;
		if ( abs( thrust ) < 0.1f ) {
			thrust = 0.0f;
		} else if ( thrust > 0.0f ) {
			thrust = ( thrust - 0.1f ) * ( 1.0f / 0.9f );
		} else if ( thrust < 0.0f ) {
			thrust = ( thrust + 0.1f ) * ( 1.0f / 0.9f );
		}
		thrust *= 0.01f; // Reduce to 1m/s
		mAltitudeControl += thrust;
		mAltitudePID.Process( mAltitudeControl, imu->altitude(), dt );
		thrust = mAltitudePID.state();
	}

	Vector3f ratePID( mRateRollPID.state(), mRatePitchPID.state(), mRateYawPID.state() );
/*
	char stmp[64];
	sprintf( stmp, "\"%.4f,%.4f,%.4f\"", ratePID.x, ratePID.y, ratePID.z );
	Main::instance()->blackbox()->Enqueue( "Stabilizer:ratePID", stmp );
*/
	if ( mFrame->Stabilize( ratePID, thrust ) == false ) {
		gDebug() << "stab error";
		Reset( mHorizonPID.state().z );
	}
}

void Stabilizer::MotorTest(uint32_t id) {
	mLockState = 1;
	while ( mLockState != 2 ) {
		usleep( 1000 * 10 );
	}

	mFrame->MotorTest(id);

	mLockState = 0;
}

void Stabilizer::CalibrateESCs()
{
	if ( mMain->imu()->state() != IMU::Off ) {
		mLockState = 1;
		while ( mLockState != 2 ) {
			usleep( 1000 * 10 );
		}
	}

	mFrame->CalibrateESCs();

	mLockState = 0;
}
