#ifndef FAKEGYROSCOPE_H
#define FAKEGYROSCOPE_H

#include <Gyroscope.h>

class FakeGyroscope : public Gyroscope
{
public:
	FakeGyroscope( int axisCount = 3, const Vector3f& noiseGain = Vector3f( 0.4f, 0.4f, 0.4f ) );
	~FakeGyroscope();

	void Read( Vector3f* v, bool raw = false );
	void Calibrate( float dt, bool last_pass = false );

	std::string infos();

protected:
	int mAxisCount;
	Vector3f mNoiseGain;
	float mSinCounter;
};

#endif // FAKEGYROSCOPE_H
