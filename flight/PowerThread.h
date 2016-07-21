/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef POWERTHREAD_H
#define POWERTHREAD_H

#include <mutex>
#include <vector>
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


	Main* mMain;
	float mLowVoltageValue;
	int32_t mLowVoltageBuzzerPin;
	std::vector< uint32_t > mLowVoltageBuzzerPattern;
	uint64_t mLowVoltageTick;
	uint32_t mLowVoltagePatternCase;

	uint64_t mSaveTicks;
	uint64_t mTicks;
	uint32_t mCellsCount;
	float mLastVBat;
	float mVBat;
	float mCurrentTotal;
	float mCurrentDraw;
	float mBatteryLevel;
	float mBatteryCapacity;

	std::mutex mCapacityMutex;
	BatterySensor mVoltageSensor;
	BatterySensor mCurrentSensor;
};


#endif // POWERTHREAD_H
