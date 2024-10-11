#ifndef MULTICOPTER_H
#define MULTICOPTER_H

#include "Frame.h"

class Main;

LUA_CLASS class Multicopter : public Frame
{
public:
	LUA_EXPORT Multicopter();
	~Multicopter();

	void Arm();
	void Disarm();
	void WarmUp();
	virtual bool Stabilize( const Vector3f& pid_output, float thrust );

protected:
	vector< float > mStabSpeeds;
	LUA_PROPERTY("maxspeed") float mMaxSpeed;
	LUA_PROPERTY("test") float mTest;
	LUA_PROPERTY("air_mode.trigger") float mAirModeTrigger;
	LUA_PROPERTY("air_mode.speed") float mAirModeSpeed;
	LUA_PROPERTY("matrix") std::vector<Vector4f> mMatrix;
};

#endif // MULTICOPTER_H
