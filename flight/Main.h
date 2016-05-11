#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "Config.h"
#include <Board.h>
#include "Debug.h"
#include "PowerThread.h"
#include "Vector.h"

class IMU;
class Stabilizer;
class Frame;
class Controller;
class Camera;

class Main
{
public:
	static int flight_entry( int ac, char** av );
	Main();
	~Main();

	Config* config() const;
	Board* board() const;
	PowerThread* powerThread() const;
	IMU* imu() const;
	Stabilizer* stabilizer() const;
	Frame* frame() const;
	Controller* controller() const;
	Camera* camera() const;

private:
	int flight_register();
	void DetectDevices();
	bool StabilizerThreadRun();

	HookThread< Main >* mStabilizerThread;
	uint32_t mLoopTime;
	uint64_t mTicks;
	uint64_t mWaitTicks;
	uint64_t mLPSTicks;
	uint32_t mLPS;

	Config* mConfig;
	Board* mBoard;
	PowerThread* mPowerThread;
	IMU* mIMU;
	Stabilizer* mStabilizer;
	Frame* mFrame;
	Controller* mController;
	Camera* mCamera;
};

#endif
