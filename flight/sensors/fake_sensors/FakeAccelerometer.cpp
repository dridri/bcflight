#include <cmath>
#include "FakeAccelerometer.h"


FakeAccelerometer::FakeAccelerometer( int axisCount, const Vector3f& noiseGain )
	: Accelerometer()
	, mAxisCount( axisCount )
	, mNoiseGain( noiseGain )
	, mSinCounter( 0.0f )
{
	mNames = { "FakeAccelerometer" };
}


FakeAccelerometer::~FakeAccelerometer()
{
}


void FakeAccelerometer::Calibrate( float dt )
{
}


void FakeAccelerometer::Read( Vector3f* v, bool raw )
{
	for ( int i = 0; i < mAxisCount; i++ ) {
		v->operator[](0) = 0.0f * std::sin( mSinCounter ) + mNoiseGain[0] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
		v->operator[](1) = mNoiseGain[1] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
		v->operator[](2) = 9.8f + mNoiseGain[2] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
	}
	mLastValues = *v;
	mSinCounter += 0.01;
}
