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
#include <Debug.h>
#include <Config.h>
#include <Controller.h>
#include "XFrame.h"
#include "IMU.h"
#include <motors/BrushlessPWM.h>
#include <Servo.h>
#include <Board.h>

int XFrame::flight_register( Main* main )
{
	RegisterFrame( "XFrame", XFrame::Instanciate );
	return 0;
}


Frame* XFrame::Instanciate( Config* config )
{
	return new XFrame( config );
}


XFrame::XFrame( Config* config )
	: Frame()
	, mStabSpeeds{ 0.0f }
	, mPIDMultipliers{ Vector3f() }
	, mMaxSpeed( 1.0f )
	, mArmed( false )
	, mAirMode( false )
	, mAirModeSpeed( config->number( "frame.air_mode.speed", 0.15f ) )
{
	mMotors.resize( 4 );

	float maxspeed = config->number( "frame.motors.max_speed" );
	if ( maxspeed > 0.0f and maxspeed <= 1.0f ) {
		mMaxSpeed = maxspeed;
	}

	int fl_pin = config->integer( "frame.motors.front_left.pin" );
	int fl_min = config->integer( "frame.motors.front_left.minimum_us", 1020 );
	int fl_max = config->integer( "frame.motors.front_left.maximum_us", 1860 );
	mMotors[0] = new BrushlessPWM( fl_pin, fl_min, fl_max );

	int fr_pin = config->integer( "frame.motors.front_right.pin" );
	int fr_min = config->integer( "frame.motors.front_right.minimum_us", 1020 );
	int fr_max = config->integer( "frame.motors.front_right.maximum_us", 1860 );
	mMotors[1] = new BrushlessPWM( fr_pin, fr_min, fr_max );

	int rl_pin = config->integer( "frame.motors.rear_left.pin" );
	int rl_min = config->integer( "frame.motors.rear_left.minimum_us", 1020 );
	int rl_max = config->integer( "frame.motors.rear_left.maximum_us", 1860 );
	mMotors[2] = new BrushlessPWM( rl_pin, rl_min, rl_max );

	int rr_pin = config->integer( "frame.motors.rear_right.pin" );
	int rr_min = config->integer( "frame.motors.rear_right.minimum_us", 1020 );
	int rr_max = config->integer( "frame.motors.rear_right.maximum_us", 1860 );
	mMotors[3] = new BrushlessPWM( rr_pin, rr_min, rr_max );


	mPIDMultipliers[0].x = config->number( "frame.motors.front_left.pid_vector.x" );
	mPIDMultipliers[0].y = config->number( "frame.motors.front_left.pid_vector.y" );
	mPIDMultipliers[0].z = config->number( "frame.motors.front_left.pid_vector.z" );
	if ( mPIDMultipliers[0].length() == 0.0f ) {
		gDebug() << "WARNING : PID multipliers for motor 0 seem to be not set !\n";
	}

	mPIDMultipliers[1].x = config->number( "frame.motors.front_right.pid_vector.x" );
	mPIDMultipliers[1].y = config->number( "frame.motors.front_right.pid_vector.y" );
	mPIDMultipliers[1].z = config->number( "frame.motors.front_right.pid_vector.z" );
	if ( mPIDMultipliers[1].length() == 0.0f ) {
		gDebug() << "WARNING : PID multipliers for motor 1 seem to be not set !\n";
	}

	mPIDMultipliers[2].x = config->number( "frame.motors.rear_left.pid_vector.x" );
	mPIDMultipliers[2].y = config->number( "frame.motors.rear_left.pid_vector.y" );
	mPIDMultipliers[2].z = config->number( "frame.motors.rear_left.pid_vector.z" );
	if ( mPIDMultipliers[2].length() == 0.0f ) {
		gDebug() << "WARNING : PID multipliers for motor 2 seem to be not set !\n";
	}

	mPIDMultipliers[3].x = config->number( "frame.motors.rear_right.pid_vector.x" );
	mPIDMultipliers[3].y = config->number( "frame.motors.rear_right.pid_vector.y" );
	mPIDMultipliers[3].z = config->number( "frame.motors.rear_right.pid_vector.z" );
	if ( mPIDMultipliers[3].length() == 0.0f ) {
		gDebug() << "WARNING : PID multipliers for motor 3 seem to be not set !\n";
	}

	mMotors[0]->Disarm();
	mMotors[1]->Disarm();
	mMotors[2]->Disarm();
	mMotors[3]->Disarm();
}


XFrame::~XFrame()
{
}


void XFrame::Arm()
{
	fDebug0();
	memset( mStabSpeeds, 0, sizeof( mStabSpeeds ) );
	mMotors[0]->setSpeed( 0.0f, true );
	mMotors[1]->setSpeed( 0.0f, true );
	mMotors[2]->setSpeed( 0.0f, true );
	mMotors[3]->setSpeed( 0.0f, true );
	mArmed = true;
}


void XFrame::Disarm()
{
	fDebug0();
	mAirMode = false;
	memset( mStabSpeeds, 0, sizeof( mStabSpeeds ) );
	mMotors[0]->Disarm();
	mMotors[1]->Disarm();
	mMotors[2]->Disarm();
	mMotors[3]->Disarm();
	mArmed = false;
}


void XFrame::WarmUp()
{
}


bool XFrame::Stabilize( const Vector3f& pid_output, const float& thrust )
{
// 	if ( not mArmed ) {
// 		return false;
// 	}
	if ( thrust >= 0.4f ) {
		mAirMode = true;
	}

	if ( mAirMode or thrust >= 0.075f ) {
		mStabSpeeds[0] = mPIDMultipliers[0] * pid_output + thrust; // Front L
		mStabSpeeds[1] = mPIDMultipliers[1] * pid_output + thrust; // Front R
		mStabSpeeds[2] = mPIDMultipliers[2] * pid_output + thrust; // Rear L
		mStabSpeeds[3] = mPIDMultipliers[3] * pid_output + thrust; // Rear R

		float motor_max = mMaxSpeed;
		float max = std::max( std::max( std::max( mStabSpeeds[0], mStabSpeeds[1] ), mStabSpeeds[2] ), mStabSpeeds[3] );
		if ( max > motor_max ) {
			float diff = max - motor_max;
			mStabSpeeds[0] -= diff;
			mStabSpeeds[1] -= diff;
			mStabSpeeds[2] -= diff;
			mStabSpeeds[3] -= diff;
		}


		if ( mAirMode ) {
			mStabSpeeds[0] = std::max( mStabSpeeds[0], mAirModeSpeed ); // TODO : Use config-defined value
			mStabSpeeds[1] = std::max( mStabSpeeds[1], mAirModeSpeed );
			mStabSpeeds[2] = std::max( mStabSpeeds[2], mAirModeSpeed );
			mStabSpeeds[3] = std::max( mStabSpeeds[3], mAirModeSpeed );
		} else if ( thrust <= 0.15f ) {
			mStabSpeeds[0] = thrust;
			mStabSpeeds[1] = thrust;
			mStabSpeeds[2] = thrust;
			mStabSpeeds[3] = thrust;
		}

// 		if ( ( Board::GetTicks() / ( 10 * 1000 ) ) % 5 == 0 ) {
// 			gDebug() << "mStabSpeeds[0] = " << mStabSpeeds[0] << "\n";
// 		}

		mMotors[0]->setSpeed( mStabSpeeds[0] );
		mMotors[1]->setSpeed( mStabSpeeds[1] );
		mMotors[2]->setSpeed( mStabSpeeds[2] );
		mMotors[3]->setSpeed( mStabSpeeds[3], true );

		return true;
	}

	mMotors[0]->setSpeed( 0.0f );
	mMotors[1]->setSpeed( 0.0f );
	mMotors[2]->setSpeed( 0.0f );
	mMotors[3]->setSpeed( 0.0f, true );
	return false;
}
