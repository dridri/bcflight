#include <Debug.h>
#include <Config.h>
#include <Controller.h>
#include "IMU.h"
#include <motors/BrushlessPWM.h>
#include <motors/OneShot125.h>
#include <Servo.h>
#include <Board.h>
#include "Multicopter.h"


Multicopter::Multicopter()
	: Frame()
	, mMaxSpeed( 1.0f )
	, mMotorSlewTime( 0.0f )
	, mAirModeTrigger( 1.0f )
	, mAirModeSpeed( 0.0f )
	// , mAirModeTrigger( config->Number( "frame.air_mode.trigger", 0.35f ) )
	// , mAirModeSpeed( config->Number( "frame.air_mode.speed", 0.15f ) )
{
	/*
	float maxspeed = config->Number( "frame.max_speed", 1.0f );
	if ( maxspeed > 0.0f and maxspeed <= 1.0f ) {
		mMaxSpeed = maxspeed;
	}

	int motors_count = config->ArrayLength( "frame.motors" );
	if ( motors_count < 3 ) {
		gDebug() << "ERROR : There should be at least 3 configured motors !";
		return;
	}
	mMotors.resize( motors_count );
	mPIDMultipliers.resize( motors_count );
	mStabSpeeds.resize( motors_count );

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		string object = "frame.motors[" + to_string(i+1) + "]";
		mMotors[i] = Motor::Instanciate( config->String( object + ".motor_type", "PWM" ), config, object );

		mPIDMultipliers[i].x = config->Number( object + ".pid_vector.x" );
		mPIDMultipliers[i].y = config->Number( object + ".pid_vector.y" );
		mPIDMultipliers[i].z = config->Number( object + ".pid_vector.z" );
		if ( mPIDMultipliers[i].length() == 0.0f ) {
			gDebug() << "WARNING : PID multipliers for motor " << (i+1) << " seem to be not set !";
		}

		mMotors[i]->Disable();
	}
	*/
}


Multicopter::~Multicopter()
{
}


void Multicopter::Arm()
{
	fDebug();

	Frame::Arm();

	char stmp[256] = "\"";
	uint32_t spos = 1;

	if ( mStabSpeeds.size() < mMotors.size() ) {
		mStabSpeeds.resize( mMotors.size() );
	}

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mStabSpeeds[i] = 0.0f;
		mMotors[i]->Arm();
		mMotors[i]->setSpeed( 0.0f, true );
		spos += sprintf( stmp + spos, "%.4f,", mMotors[i]->speed() );
	}

	mArmed = true;
	Main::instance()->blackbox()->Enqueue( "Multicopter:motors", string(stmp) + "\"" );
}


void Multicopter::Disarm()
{
	fDebug();

	char stmp[256] = "\"";
	uint32_t spos = 1;

	mAirMode = false;

	if ( mStabSpeeds.size() < mMotors.size() ) {
		mStabSpeeds.resize( mMotors.size() );
	}

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mStabSpeeds[i] = 0.0f;
		mMotors[i]->setSpeed( 0.0f, true );
	}

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mMotors[i]->Disarm();
		spos += sprintf( stmp + spos, "%.4f,", mMotors[i]->speed() );
	}

	mArmed = false;
	Main::instance()->blackbox()->Enqueue( "Multicopter:motors", string(stmp) + "\"" );
}


void Multicopter::WarmUp()
{
	fDebug();
}


bool Multicopter::Stabilize( const Vector3f& pid_output, float thrust, float dt )
{
	if ( not mArmed ) {
		return false;
	}
	if ( thrust >= mAirModeTrigger ) {
		if ( not mAirMode ) {
			gDebug() << "Now in air mode";
		}
		mAirMode = true;
	}

	if ( mAirMode or thrust >= 0.0f ) {
		float expected_min = ( mAirMode ? mAirModeSpeed : 0.0f );
		float expected_max = mMaxSpeed;
		float overall_min = 1e6f;
		float overall_max = -1e6f;

		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			mStabSpeeds[i] = ( mMatrix[i].xyz() & pid_output ) + mMatrix[i].w * thrust;
			overall_min = std::min( overall_min, mStabSpeeds[i] );
			overall_max = std::max( overall_max, mStabSpeeds[i] );
		}

		if ( overall_min < expected_min ) {
			for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
				mStabSpeeds[i] += expected_min - overall_min;
			}
			overall_max += expected_min - overall_min;
			overall_min = expected_min;
		}

		float stab_multiplier = 1.0f;
		if ( overall_max > expected_max ) {
			stab_multiplier = ( expected_max ) / ( std::max(1.0f, overall_max) - std::min(0.0f, overall_min) );
		}

		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			float v = mStabSpeeds[i];
			mStabSpeeds[i] = expected_min + ( mStabSpeeds[i] - expected_min ) * stab_multiplier;
			// Apply motor slew rate limiting if enabled (mMotorSlewTime in ms)
			if ( mMotorSlewTime > 0.0f && dt > 0.0f ) {
				float currentSpeed = mMotors[i]->speed();
				float maxChange = dt * 1000.0f / mMotorSlewTime;
				float delta = std::clamp( mStabSpeeds[i] - currentSpeed, -maxChange, maxChange );
				mStabSpeeds[i] = currentSpeed + delta;
			}
			mMotors[i]->setSpeed( mStabSpeeds[i], ( i >= mMotors.size() - 1 ) );
		}

		if ( Main::instance()->blackbox() ) {
			char stmp[256] = "\"";
			uint32_t spos = 1;
			for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
				spos += sprintf( stmp + spos, "%.4f,", mMotors[i]->speed() );
			}
			Main::instance()->blackbox()->Enqueue( "Multicopter:motors", string(stmp) + "\"" );
		}

		return true;
	}

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mMotors[i]->setSpeed( 0.0f, ( i >= mMotors.size() - 1 ) );
	}
	return false;
}
