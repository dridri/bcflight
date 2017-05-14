#include <Debug.h>
#include <Config.h>
#include <Controller.h>
#include "IMU.h"
#include <motors/BrushlessPWM.h>
#include <motors/OneShot125.h>
#include <Servo.h>
#include <Board.h>
#include "Multicopter.h"

int Multicopter::flight_register( Main* main )
{
	RegisterFrame( "Multicopter", Multicopter::Instanciate );
	return 0;
}


Frame* Multicopter::Instanciate( Config* config )
{
	return new Multicopter( config );
}

Multicopter::Multicopter( Config* config )
	: Frame()
	, mMaxSpeed( 1.0f )
	, mAirModeSpeed( config->number( "frame.air_mode.speed", 0.15f ) )
{
	float maxspeed = config->number( "frame.max_speed" );
	if ( maxspeed > 0.0f and maxspeed <= 1.0f ) {
		mMaxSpeed = maxspeed;
	}

	int motors_count = config->ArrayLength( "frame.motors" );
	if ( motors_count < 3 ) {
		gDebug() << "ERROR : There should be at least 3 configured motors !\n";
		return;
	}
	mMotors.resize( motors_count );
	mPIDMultipliers.resize( motors_count );
	mStabSpeeds.resize( motors_count );

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		int fl_pin = config->integer( "frame.motors[" + std::to_string(i+1) + "].pin" );
		int fl_min = config->integer( "frame.motors[" + std::to_string(i+1) + "].minimum_us", 1020 );
		int fl_max = config->integer( "frame.motors[" + std::to_string(i+1) + "].maximum_us", 1860 );
		mMotors[i] = new BrushlessPWM( fl_pin, fl_min, fl_max );
// 		mMotors[i] = new OneShot125( fl_pin );

		mPIDMultipliers[i].x = config->number( "frame.motors[" + std::to_string(i+1) + "].pid_vector.x" );
		mPIDMultipliers[i].y = config->number( "frame.motors[" + std::to_string(i+1) + "].pid_vector.y" );
		mPIDMultipliers[i].z = config->number( "frame.motors[" + std::to_string(i+1) + "].pid_vector.z" );
		if ( mPIDMultipliers[i].length() == 0.0f ) {
			gDebug() << "WARNING : PID multipliers for motor " << (i+1) << " seem to be not set !\n";
		}

		mMotors[i]->Disable();
	}
}


Multicopter::~Multicopter()
{
}


void Multicopter::Arm()
{
	fDebug0();
	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mStabSpeeds[i] = 0.0f;
		mMotors[i]->setSpeed( 0.0f, true );
	}
	mArmed = true;
}


void Multicopter::Disarm()
{
	fDebug0();
	mAirMode = false;
	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mMotors[i]->Disarm();
		mStabSpeeds[i] = 0.0f;
	}
	mArmed = false;
}


void Multicopter::WarmUp()
{
}


bool Multicopter::Stabilize( const Vector3f& pid_output, const float& thrust )
{
	if ( not mArmed ) {
		return false;
	}
	// Automatically enable air-mode after lift-off
	if ( thrust >= 0.35f ) {
		mAirMode = true;
	}

	if ( mAirMode or thrust >= 0.075f ) {
		float overall_min = 0.0f;
		float overall_max = 1.0f;
		float stab_shift = 0.0f;
		float stab_multiplier = 0.0f;

		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			mStabSpeeds[i] = mPIDMultipliers[i] * pid_output + thrust;
			overall_min = std::min( overall_min, mStabSpeeds[i] );
			overall_max = std::max( overall_max, mStabSpeeds[i] );
		}
		if ( mAirMode ) {
			stab_multiplier = ( 1.0f - mAirModeSpeed ) / ( overall_max - overall_min );
			stab_shift = mAirModeSpeed;
		} else {
			stab_multiplier = 1.0f / ( overall_max - overall_min );
		}

		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			mStabSpeeds[i] = stab_shift + ( mStabSpeeds[i] - overall_min ) * stab_multiplier;
		}

		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			mMotors[i]->setSpeed( mStabSpeeds[i], ( i >= mMotors.size() - 1 ) );
		}

		return true;
	}

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mMotors[i]->setSpeed( 0.0f, ( i >= mMotors.size() - 1 ) );
	}
	return false;
}
