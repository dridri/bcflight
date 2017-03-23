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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <mutex>
#include <Thread.h>
#include "Vector.h"

class Main;
class Link;

class Controller : public Thread
{
public:
	Controller( Main* main, Link* link );
	~Controller();

	bool connected() const;
	bool armed() const;
	float thrust() const;
	const Vector3f& RPY() const;

	void UpdateSmoothControl( const float& dt );
	void Emergency();

	void SendDebug( const std::string& s );

protected:
	typedef enum {
		// Configure
		PING = 0x70,
		CALIBRATE = 0x71,
		SET_TIMESTAMP = 0x72,
		ARM = 0x73,
		DISARM = 0x74,
		RESET_BATTERY = 0x75,
		CALIBRATE_ESCS = 0x76,
		SET_FULL_TELEMETRY = 0x77,
		DEBUG_OUTPUT = 0x7A,
		GET_BOARD_INFOS = 0x80,
		GET_SENSORS_INFOS = 0x81,
		GET_CONFIG_FILE = 0x90,
		SET_CONFIG_FILE = 0x91,
		UPDATE_UPLOAD_INIT = 0x9A,
		UPDATE_UPLOAD_DATA = 0x9B,
		UPDATE_UPLOAD_PROCESS = 0x9C,
		ENABLE_TUN_DEVICE = 0x9E,
		DISABLE_TUN_DEVICE = 0x9F,
		// Getters
		PRESSURE = 0x10,
		TEMPERATURE = 0x11,
		ALTITUDE = 0x12,
		ROLL = 0x13,
		PITCH = 0x14,
		YAW = 0x15,
		ROLL_PITCH_YAW = 0x16,
		ACCEL = 0x17,
		GYRO = 0x18,
		MAGN = 0x19,
		MOTORS_SPEED = 0x1A,
		CURRENT_ACCELERATION = 0x1B,
		SENSORS_DATA = 0x20,
		PID_OUTPUT = 0x21,
		OUTER_PID_OUTPUT = 0x22,
		ROLL_PID_FACTORS = 0x23,
		PITCH_PID_FACTORS = 0x24,
		YAW_PID_FACTORS = 0x25,
		OUTER_PID_FACTORS = 0x26,
		HORIZON_OFFSET = 0x27,
		VBAT = 0x30,
		TOTAL_CURRENT = 0x31,
		CURRENT_DRAW = 0x32,
		BATTERY_LEVEL = 0x34,
		CPU_LOAD = 0x35,
		CPU_TEMP = 0x36,
		RX_QUALITY = 0x37,
		STABILIZER_FREQUENCY = 0x38,
		// Setters
		SET_ROLL = 0x40,
		SET_PITCH = 0x41,
		SET_YAW = 0x42,
		SET_THRUST = 0x43,
		RESET_MOTORS = 0x47,
		SET_MODE = 0x48,
		SET_ALTITUDE_HOLD = 0x49,
		SET_ROLL_PID_P = 0x50,
		SET_ROLL_PID_I = 0x51,
		SET_ROLL_PID_D = 0x52,
		SET_PITCH_PID_P = 0x53,
		SET_PITCH_PID_I = 0x54,
		SET_PITCH_PID_D = 0x55,
		SET_YAW_PID_P = 0x56,
		SET_YAW_PID_I = 0x57,
		SET_YAW_PID_D = 0x58,
		SET_OUTER_PID_P = 0x59,
		SET_OUTER_PID_I = 0x5A,
		SET_OUTER_PID_D = 0x5B,
		SET_HORIZON_OFFSET = 0x5C,
		// Video
		VIDEO_PAUSE = 0xA0,
		VIDEO_RESUME = 0xA1,
		VIDEO_START_RECORD = 0xA2,
		VIDEO_STOP_RECORD = 0xA3,
		VIDEO_BRIGHTNESS_INCR = 0xA4,
		VIDEO_BRIGHTNESS_DECR = 0xA5,
		VIDEO_CONTRAST_INCR = 0xA6,
		VIDEO_CONTRAST_DECR = 0xA7,
		VIDEO_SATURATION_INCR = 0xA8,
		VIDEO_SATURATION_DECR = 0xA9,
		GET_RECORDINGS_LIST = 0xB1,
		RECORD_DOWNLOAD = 0xB2,
		RECORD_DOWNLOAD_INIT = 0xB3,
		RECORD_DOWNLOAD_DATA = 0xB4,
		RECORD_DOWNLOAD_PROCESS = 0xB5,
		VIDEO_NIGHT_MODE = 0xC0,
		// Errors - 0x7Fxxxxxx
		// Host system errors - 0x7F001xxx
		// Camera/video errors - 0x7F00Fxxx
		CAMERA_MISSING = 0x7F00F000,
		MOTOR_TEST = 0xD0
	} Cmd;

	virtual bool run();
	bool TelemetryRun();
	uint32_t crc32( const uint8_t* buf, uint32_t len );

	Main* mMain;
	Link* mLink;
	std::mutex mSendMutex;
	bool mConnected;
	bool mArmed;
	Vector4f mExpo;
	Vector3f mRPY;
	float mThrust;
	float mThrustAccum;
	Vector3f mSmoothRPY;
	uint64_t mTicks;
	HookThread< Controller >* mTelemetryThread;
	uint64_t mTelemetryTick;
	uint64_t mTelemetryCounter;
	uint64_t mEmergencyTick;
	uint32_t mTelemetryFrequency;
	bool mTelemetryFull;
};

#endif // CONTROLLER_H
