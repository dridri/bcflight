#ifndef POWERTHREAD_H
#define POWERTHREAD_H

#include <Thread.h>

class Main;
class Sensor;

class PowerThread : public Thread
{
public:
	PowerThread( Main* main );
	~PowerThread();

	void ResetFullBattery( uint32_t capacity_mah = 2500 );

	float VBat() const;
	float CurrentTotal() const;
	float CurrentDraw() const;
	float BatteryLevel() const;

protected:
	virtual bool run();

private:
	typedef enum {
		NONE = 0,
		VOLTAGE,
		CURRENT
	} SensorType;
	typedef struct {
		SensorType type;
		Sensor* sensor;
		int channel;
		float shift;
		float multiplier;
	} BatterySensor;

	uint64_t mTicks;
	Main* mMain;
	float mLastVBat;
	float mVBat;
	float mCurrentTotal;
	float mCurrentDraw;
	float mBatteryLevel;
	float mBatteryCapacity;

	BatterySensor mVoltageSensor;
	BatterySensor mCurrentSensor;
};


#endif // POWERTHREAD_H
