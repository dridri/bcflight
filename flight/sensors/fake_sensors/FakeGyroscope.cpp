#include <cmath>
#include "FakeGyroscope.h"

FakeGyroscope::FakeGyroscope( int axisCount, const Vector3f& noiseGain )
	: Gyroscope()
	, mAxisCount( axisCount )
	, mNoiseGain( noiseGain )
	, mSinCounter( 0.0f )
{
	mNames = { "FakeGyroscope" };
}


FakeGyroscope::~FakeGyroscope()
{
}


void FakeGyroscope::Calibrate( float dt )
{
}


void FakeGyroscope::Read( Vector3f* v, bool raw )
{
	for ( int i = 0; i < mAxisCount; i++ ) {
		v->operator[](0) = 0.0f * std::sin( mSinCounter + 3.14 / 2 ) + mNoiseGain[0] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
		v->operator[](1) = mNoiseGain[1] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
		v->operator[](2) = mNoiseGain[2] * ( (float)( rand() % 65536 ) - 32768.0f ) / 32768.0f;
	}
	mLastValues = *v;
	mSinCounter += 0.1;
}
