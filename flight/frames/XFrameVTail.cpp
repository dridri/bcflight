#include <unistd.h>
#include <Debug.h>
#include <Config.h>
#include <Controller.h>
#include "XFrameVTail.h"
#include "IMU.h"
#include <Servo.h>

int XFrameVTail::flight_register( Main* main )
{
	RegisterFrame( "XFrameVTail", XFrameVTail::Instanciate );
	return 0;
}


Frame* XFrameVTail::Instanciate( Config* config )
{
	return new XFrameVTail( config );
}

XFrameVTail::XFrameVTail( Config* config )
	: XFrame( config )
{
	// Motors configuration already loaded by XFrame
}


XFrameVTail::~XFrameVTail()
{
}


void XFrameVTail::Stabilize( const Vector3f& pid_output, const float& thrust )
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
