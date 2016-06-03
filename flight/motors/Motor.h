#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

class Motor
{
public:
	Motor();
	virtual ~Motor();

	const float speed() const;
	void setSpeed( float speed, bool force_hw_update = false );
	void KeepSpeed();
	virtual void Disarm() = 0;
	virtual void Disable() = 0;

protected:
	virtual void setSpeedRaw( float speed, bool force_hw_update = false ) = 0;

	float mSpeed;
};

#endif // MOTOR_H
