#ifndef POWERTHREAD_H
#define POWERTHREAD_H

#include <Thread.h>

class Main;

class PowerThread : public Thread
{
public:
	PowerThread( Main* main );
	~PowerThread();

	void ResetFullBattery( uint32_t capacity_mah = 2500 );

	float VBat() const;
	float CurrentTotal() const;
	float CurrentDraw() const;

protected:
	virtual bool run();

private:
	uint64_t mTicks;
	Main* mMain;
	float mLastVBat;
	float mVBat;
	float mCurrentTotal;
	float mCurrentDraw;
	float mBatteryCapacity;
};


#endif // POWERTHREAD_H
