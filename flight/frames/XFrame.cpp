#include <unistd.h>
#include <Debug.h>
#include <Controller.h>
#include "XFrame.h"
#include "IMU.h"
#include <Servo.h>

XFrame::XFrame( Motor* fl, Motor* fr, Motor* rl, Motor* rr )
	: Frame()
	, mStabSpeeds{ 0.0f }
{
	mMotors.resize( 4 );
	mMotors[0] = fl;
	mMotors[1] = fr;
	mMotors[2] = rl;
	mMotors[3] = rr;

// 	mMotors[0]->setSpeed( 0.0f, true );
// 	mMotors[1]->setSpeed( 0.0f, true );
// 	mMotors[2]->setSpeed( 0.0f, true );
// 	mMotors[3]->setSpeed( 0.0f, true );
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
}


void XFrame::Disarm()
{
	fDebug0();
	memset( mStabSpeeds, 0, sizeof( mStabSpeeds ) );
	mMotors[0]->Disarm();
	mMotors[1]->Disarm();
	mMotors[2]->Disarm();
	mMotors[3]->Disarm();
	Servo::HardwareSync();
}


void XFrame::WarmUp()
{
/*
	mMotors[0]->setSpeed( 0.03f );
	mMotors[1]->setSpeed( 0.03f );
	mMotors[2]->setSpeed( 0.03f );
	mMotors[3]->setSpeed( 0.03f );
*/
}


void XFrame::Stabilize( const Vector3f& pid_output, const float& thrust )
{
	mStabSpeeds[0] = Vector3f( +1.0f, -2.0f/3.0f, +0.0f ) * pid_output + thrust; // Front L
	mStabSpeeds[1] = Vector3f( -1.0f, -2.0f/3.0f, -0.0f ) * pid_output + thrust; // Front R
	mStabSpeeds[2] = Vector3f( +1.0f/5.0f, +4.0f/3.0f, +1.0f ) * pid_output + thrust; // Rear L
	mStabSpeeds[3] = Vector3f( -1.0f/5.0f, +4.0f/3.0f, -1.0f ) * pid_output + thrust; // Rear R
/*
	float min = std::min( std::min( std::min( mStabSpeeds[0], mStabSpeeds[1] ), mStabSpeeds[2] ), mStabSpeeds[3] );
	float motor_min = 0.18f;
	if ( min < motor_min and thrust > min ) {
		float diff = motor_min - min;
		mStabSpeeds[0] += diff;
		mStabSpeeds[1] += diff;
		mStabSpeeds[2] += diff;
		mStabSpeeds[3] += diff;
	}
*/

	float motor_max = 1.0f;
	float max = std::max( std::max( std::max( mStabSpeeds[0], mStabSpeeds[1] ), mStabSpeeds[2] ), mStabSpeeds[3] );
	if ( max > motor_max ) {
		float diff = max - motor_max;
		mStabSpeeds[0] -= diff;
		mStabSpeeds[1] -= diff;
		mStabSpeeds[2] -= diff;
		mStabSpeeds[3] -= diff;
	}

	mMotors[0]->setSpeed( mStabSpeeds[0] );
	mMotors[1]->setSpeed( mStabSpeeds[1] );
	mMotors[2]->setSpeed( mStabSpeeds[2] );
	mMotors[3]->setSpeed( mStabSpeeds[3] );
	Servo::HardwareSync();
}
