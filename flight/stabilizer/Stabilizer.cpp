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


constexpr float lerp(float a, float b, float t) {
	return a + t * (b - a);
}


Stabilizer::Stabilizer()
	: mMain( Main::instance() )
	, mFrame( nullptr )
	, mMode( Rate )
	, mAltitudeHold( false )
	, mRateRollPID( PID<float>() )
	, mRatePitchPID( PID<float>() )
	, mRateYawPID( PID<float>() )
	, mRollHorizonPID( PID<float>() )
	, mPitchHorizonPID( PID<float>() )
	, mAltitudePID( PID<float>() )
	, mAltitudeControl( 0.0f )
	, mDerivativeFilter( nullptr )
	, mTPAMultiplier( 1.0f )
	, mTPAThreshold( 1.0f )
	, mAntiGravityGain( 1.0f )
	, mAntiGravityThreshold( 0.0f )
	, mAntiGravityDecay( 10.0f )
	, mAntigravityThrustAccum( 0.0f )
	, mAntiWindupThreshold( 0.85f )
	, mAntiWindupFactor( 1.0f )
	, mArmed( false )
	, mFilteredRPYDerivative( Vector3f() )
	, mLockState( 0 )
	, mHorizonMultiplier( Vector3f( 15.0f, 15.0f, 1.0f ) )
	, mHorizonOffset( Vector3f() )
	, mHorizonMaxRate( Vector3f( 300.0f, 300.0f, 300.0f ) )
{
	fDebug(this);

	mAltitudePID.setP( 0.001 );
	mAltitudePID.setI( 0.010 );
	mAltitudePID.setDeadBand( 0.05f );
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
	mRollHorizonPID.Reset();
	mPitchHorizonPID.Reset();
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
	mPreviousThrust = mThrust;
	mThrust = value;
}


void Stabilizer::Update( IMU* imu, float dt )
{
	Vector3f rate_control = Vector3f();

	if ( mLockState >= 1 ) {
		mLockState = 2;
		return;
	}

	Vector3f rates = imu->rate();

	if ( mDerivativeFilter ) {
		mFilteredRPYDerivative = mDerivativeFilter->filter( rates, dt );
	} else {
		mFilteredRPYDerivative = rates;
	}

	if ( not mArmed ) {
		return;
	}

	// We didn't take off yet (or really close to ground just after take-off), so we bypass stabilization, and just return
	if ( mFrame->airMode() == false and mThrust <= 0.15f ) {
		mFrame->Stabilize( Vector3f( 0.0f, 0.0f, 0.0f ), mThrust, dt );
		return;
	}

	Vector3f rollPIDMultiplier = Vector3f( 1.0f, 1.0f, 1.0f );
	Vector3f pitchPIDMultiplier = Vector3f( 1.0f, 1.0f, 1.0f );
	Vector3f yawPIDMultiplier = Vector3f( 1.0f, 1.0f, 1.0f );

	// Throttle PID Attenuation (TPA) : reduce PID gains when throttle is high
	// y=min( 1, 1 − ( (x−t) / (1−t) ) * m )
	// See https://www.desmos.com/calculator/wi8qeuzct6
	if ( mTPAThreshold > 0.0f and mTPAThreshold < 1.0f ) {
		float tpa = ( ( mThrust - mTPAThreshold ) / ( 1.0f - mTPAThreshold ) ) * mTPAMultiplier;
		Vector3f tpaPID = Vector3f(
			1.0f - 0.25f * std::clamp( tpa, 0.0f, 1.0f ),
			1.0f - 0.25f * std::clamp( tpa, 0.0f, 1.0f ),
			1.0f - std::clamp( tpa, 0.0f, 1.0f )
		);
		rollPIDMultiplier = rollPIDMultiplier * tpaPID;
		pitchPIDMultiplier = pitchPIDMultiplier * tpaPID;
		yawPIDMultiplier = yawPIDMultiplier * tpaPID;
	}

	// Anti-gravity
	if ( mAntiGravityThreshold > 0.0f ) {
		float delta = std::abs( mThrust - mPreviousThrust ) / dt;
		mAntigravityThrustAccum = std::min( 1.0f, mAntigravityThrustAccum * ( 1.0f - dt * mAntiGravityDecay ) + delta * dt );
		float ag = 1.0f + (mAntiGravityGain - 1.0f) * std::max(0.0f, std::min( 1.0f, ( mAntigravityThrustAccum - mAntiGravityThreshold ) / (1.0f - mAntiGravityThreshold) ) );
		rollPIDMultiplier.y *= ag;
		pitchPIDMultiplier.y *= ag;
		yawPIDMultiplier.y *= ag;
		if ( ag > 1.0f ) {
			gTrace() << "AG: " << ag << " | " << mAntigravityThrustAccum << " | " << delta;
		}
	}

	switch ( mMode ) {
		case ReturnToHome :
		case Follow :
		case Stabilize : {
			Vector3f drone_state = imu->RPY();
			Vector3f control_angles = mRPY;
			control_angles.x = mHorizonMultiplier.x * min( max( control_angles.x, -1.0f ), 1.0f ) + mHorizonOffset.x;
			control_angles.y = mHorizonMultiplier.y * min( max( control_angles.y, -1.0f ), 1.0f ) + mHorizonOffset.y;
			// TODO : when user-input is 0, set control_angles by using imu->velocity().xy to compensate position drifting, if enabled
			mRollHorizonPID.Process( control_angles.x, drone_state.x, dt );
			mPitchHorizonPID.Process( control_angles.y, drone_state.y, dt );
			rate_control.x = mRollHorizonPID.state();
			rate_control.y = mPitchHorizonPID.state();
			rate_control.x = max( -mHorizonMaxRate.x, min( mHorizonMaxRate.x, rate_control.x ) );
			rate_control.y = max( -mHorizonMaxRate.y, min( mHorizonMaxRate.y, rate_control.y ) );
			rate_control.z = mRPY.z * mRateFactor; // TEST : Bypass heading for now
			break;
		}
		case Rate :
		default : {
			rate_control = mRPY * mRateFactor;
			break;
		}
	}

	std::vector<float> motorSpeeds;
	for ( const auto& motor : *mFrame->motors() ) {
		motorSpeeds.push_back( motor->speed() );
	}

	// Anti-windup with progressive transition
	// No effect below 85%, linear transition from 85% to 100%
	float satHigh = *std::max_element( motorSpeeds.begin(), motorSpeeds.end() );
	float satLow = *std::min_element( motorSpeeds.begin(), motorSpeeds.end() );
	float motorSaturation = std::clamp( std::max( satHigh, 1.0f - satLow ), 0.0f, 1.0f );
	float antiWindupValue = 1.0f;
	if ( motorSaturation > mAntiWindupThreshold ) {
		float saturationFactor = ( motorSaturation - mAntiWindupThreshold ) / ( 1.0f - mAntiWindupThreshold );
		antiWindupValue = 1.0f - saturationFactor;  // Linear from 1.0 at mAntiWindupThreshold (default 85%) to 0.0 at 100%
	}
	float deltaR = rate_control.x - rates.x;
	float deltaP = rate_control.y - rates.y;
	float deltaY = rate_control.z - rates.z;
	float deltaRi = deltaR * lerp( 1.0f, antiWindupValue, mAntiWindupFactor );
	float deltaPi = deltaP * lerp( 1.0f, antiWindupValue, mAntiWindupFactor );
	float deltaYi = deltaY * lerp( 1.0f, antiWindupValue, mAntiWindupFactor );
	float deltaRd = rate_control.x - mFilteredRPYDerivative.x;
	float deltaPd = rate_control.y - mFilteredRPYDerivative.y;
	float deltaYd = rate_control.z - mFilteredRPYDerivative.z;
	mRateRollPID.Process( deltaR, deltaRi, deltaRd, dt, rollPIDMultiplier );
	mRatePitchPID.Process( deltaP, deltaPi, deltaPd, dt, pitchPIDMultiplier );
	mRateYawPID.Process( deltaY, deltaYi, deltaYd, dt, yawPIDMultiplier );

	float thrust = mThrust;
/*
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
*/
	Vector3f ratePID( mRateRollPID.state(), mRatePitchPID.state(), mRateYawPID.state() );
	Vector3f rateIntegral( mRateRollPID.integral(), mRatePitchPID.integral(), mRateYawPID.integral() );

	const std::string keys[4] = { "Stabilizer:ratePID", "Stabilizer:rateIntegral", "Stabilizer:extras" };
	const Vector4f values[4] = { ratePID, rateIntegral, Vector3f( mAntigravityThrustAccum, motorSaturation, antiWindupValue ) };
	Main::instance()->blackbox()->Enqueue( keys, values, 3 );

	if ( mFrame->Stabilize( ratePID, thrust, dt ) == false ) {
		gDebug() << "stab error";
		Reset( imu->RPY().z );
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
