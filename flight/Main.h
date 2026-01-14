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

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <Board.h>
#include "Debug.h"
#include "PowerThread.h"
#include "BlackBox.h"
#include "Vector.h"
#include "string"

using namespace STD;

class Config;
class Console;
class IMU;
class Stabilizer;
class Frame;
class Controller;
class Camera;
class HUD;
class Microphone;
class Recorder;

class Main
{
public:
	static int flight_entry( int ac, char** av );
	Main( int ac = 0, char** av = nullptr );
	~Main();

	const bool ready() const;

	string getRecordingsList() const;

	uint32_t loopFrequency() const;
	Config* config() const;
	Board* board() const;
	PowerThread* powerThread() const;
	BlackBox* blackbox() const;
	IMU* imu() const;
	Stabilizer* stabilizer() const;
	Frame* frame() const;
	Controller* controller() const;
	Camera* camera() const;
	HUD* hud() const;
	Microphone* microphone() const;
	const string& cameraType() const;
	const string& username() const;

	static string base64_encode( const uint8_t* buf, uint32_t size );
	static Main* instance();

private:
	int flight_register();
	void DetectDevices();
	bool StabilizerThreadRun();

	static Main* mInstance;
	bool mReady;
	HookThread< Main >* mStabilizerThread;
	uint32_t mLoopTime;
	uint64_t mTicks;
	uint64_t mWaitTicks;
	uint64_t mLPSTicks;
	uint32_t mLPS;
	uint32_t mLPSCounter;

	Config* mConfig;
	Console* mConsole;
	Board* mBoard;
	PowerThread* mPowerThread;
	BlackBox* mBlackBox;
	IMU* mIMU;
	Stabilizer* mStabilizer;
	Frame* mFrame;
	Controller* mController;
	Camera* mCamera;
	HUD* mHUD;
	Microphone* mMicrophone;
	Recorder* mRecorder;
	string mCameraType;
	string mUsername;
};

#endif
