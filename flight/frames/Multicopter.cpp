#include <Debug.h>
#include <Config.h>
#include <Controller.h>
#include "IMU.h"
#include <motors/BrushlessPWM.h>
#include <motors/OneShot125.h>
#include <Servo.h>
#include <Board.h>
#include "Multicopter.h"

#define min( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define max( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

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
	, mAirModeTrigger( config->Number( "frame.air_mode.trigger", 0.35f ) )
	, mAirModeSpeed( config->Number( "frame.air_mode.speed", 0.15f ) )
{
	float maxspeed = config->Number( "frame.max_speed", 1.0f );
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
		string object = "frame.motors[" + to_string(i+1) + "]";
		mMotors[i] = Motor::Instanciate( config->String( object + ".motor_type", "PWM" ), config, object );

		mPIDMultipliers[i].x = config->Number( object + ".pid_vector.x" );
		mPIDMultipliers[i].y = config->Number( object + ".pid_vector.y" );
		mPIDMultipliers[i].z = config->Number( object + ".pid_vector.z" );
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
	char stmp[256] = "\"";
	uint32_t spos = 1;

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mStabSpeeds[i] = 0.0f;
		mMotors[i]->setSpeed( 0.0f, true );
		spos += sprintf( stmp + spos, "%.4f,", mMotors[i]->speed() );
	}

	mArmed = true;
	Main::instance()->blackbox()->Enqueue( "Multicopter:motors", string(stmp) + "\"" );
}


void Multicopter::Disarm()
{
	fDebug0();
	char stmp[256] = "\"";
	uint32_t spos = 1;

	mAirMode = false;

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mMotors[i]->setSpeed( 0.0f, false );
		mMotors[i]->Disarm();
		mStabSpeeds[i] = 0.0f;
		spos += sprintf( stmp + spos, "%.4f,", mMotors[i]->speed() );
	}

	mArmed = false;
	Main::instance()->blackbox()->Enqueue( "Multicopter:motors", string(stmp) + "\"" );
}


void Multicopter::WarmUp()
{
}


bool Multicopter::Stabilize( const Vector3f& pid_output, const float& thrust )
{
	if ( not mArmed ) {
		return false;
	}
	if ( thrust >= mAirModeTrigger ) {
		if ( not mAirMode ) {
			gDebug() << "Now in air mode\n";
		}
		mAirMode = true;
	}

	if ( mAirMode or thrust >= 0.075f ) {
		float overall_min = 0.0f;
		float overall_max = 1.0f;
		float stab_shift = 0.0f;
		float stab_multiplier = 0.0f;

		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			mStabSpeeds[i] = mPIDMultipliers[i] * pid_output + thrust;
			overall_min = min( overall_min, mStabSpeeds[i] );
			overall_max = max( overall_max, mStabSpeeds[i] );
		}
		if ( mAirMode ) {
			stab_multiplier = ( 1.0f - mAirModeSpeed ) / ( overall_max - overall_min );
			stab_shift = mAirModeSpeed;
		} else {
			stab_multiplier = 1.0f / ( overall_max - overall_min );
		}

		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			mStabSpeeds[i] = stab_shift + max( 0.0f, ( mStabSpeeds[i] - overall_min ) * stab_multiplier );
		}

		char stmp[256] = "\"";
		uint32_t spos = 1;
		for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
			mMotors[i]->setSpeed( min( mMaxSpeed, mStabSpeeds[i] ), ( i >= mMotors.size() - 1 ) );
			spos += sprintf( stmp + spos, "%.4f,", mMotors[i]->speed() );
		}
		Main::instance()->blackbox()->Enqueue( "Multicopter:motors", string(stmp) + "\"" );

		return true;
	}

	for ( uint32_t i = 0; i < mMotors.size(); i++ ) {
		mMotors[i]->setSpeed( 0.0f, ( i >= mMotors.size() - 1 ) );
	}
	return false;
}
