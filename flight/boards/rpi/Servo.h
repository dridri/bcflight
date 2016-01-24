#ifndef SERVO_H
#define SERVO_H

class Servo
{
public:
	Servo( int pin, int us_min = 1060, int us_max = 1860 );
	virtual ~Servo();
// 	void setValue( int width_ms );
	void setValue( float p, bool force_hw_update = false );
	void Disarm();

	static void HardwareSync();

private:
	int mID;
	int mMin;
	int mMax;
	int mRange;
};

#endif // SERVO_H
