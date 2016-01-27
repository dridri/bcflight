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

	Board* board() const;
	PowerThread* powerThread() const;
	IMU* imu() const;
	Stabilizer* stabilizer() const;
	Frame* frame() const;

private:
	int flight_register();
	void DetectDevices();

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
