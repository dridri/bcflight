#ifndef FAKEACCELEROMETER_H
#define FAKEACCELEROMETER_H

#include <Accelerometer.h>

class FakeAccelerometer : public Accelerometer
{
public:
	FakeAccelerometer( int axisCount = 3, const Vector3f& noiseGain = Vector3f( 0.4f, 0.4f, 0.4f ) );
	~FakeAccelerometer();

	void Read( Vector3f* v, bool raw = false );
	void Calibrate( float dt, bool last_pass = false );

protected:
	int mAxisCount;
	Vector3f mNoiseGain;
	float mSinCounter;
};

#endif // FAKEACCELEROMETER_H
