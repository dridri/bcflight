#ifndef SERVO_H
#define SERVO_H

class Servo
{
public:
	Servo( int pin );
	virtual ~Servo();
	void setValue( int width_ms );

private:
};

#endif // SERVO_H
