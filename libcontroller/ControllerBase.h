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

#ifndef CONTROLLERBASE_H
#define CONTROLLERBASE_H

#include <unistd.h>
#include <mutex>
#include <list>
#include <Link.h>

class ControllerBase
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	ControllerBase( Link* link );
	virtual ~ControllerBase();
	bool isConnected() const { return ( mLink and mLink->isConnected() and mConnectionEstablished ); }
	Link* link() const { return mLink; }

	void Lock() { printf( "Locking controller...\n" ); mLockState = 1; while ( mLockState != 2 ) { usleep(1); } printf( "Controller lock ok...\n" ); }
	void Unlock() { mLockState = 0; }

protected:
#define STATUS_ARMED 1
#define STATUS_CALIBRATED 2
#define STATUS_CALIBRATING 4
#define STATUS_NIGHTMODE 128

	typedef struct __attribute__((packed)) Controls {
		int8_t thrust;
		int8_t roll;
		int8_t pitch;
		int8_t yaw;
		int8_t arm : 1;
		int8_t stabilizer_mode : 1;
		int8_t night_mode : 1;
		int8_t take_picture : 1;
		int8_t _unused : 4;
	} Controls;

	typedef struct __attribute__((packed)) Telemetry {
		uint16_t battery_voltage;
		uint16_t total_current;
		uint8_t current_draw;
		int8_t battery_level;
		uint8_t cpu_load;
		uint8_t cpu_temp;
		int8_t rx_quality;
		int8_t rx_level;
	} Telemetry;

#define SHORT_COMMAND 0xF0

	typedef enum {
		UNKNOWN = 0,

		// Continuous data (should be as small as possible)
		STATUS = SHORT_COMMAND | 0x0,
		PING = SHORT_COMMAND | 0x1,
		TELEMETRY = SHORT_COMMAND | 0x2,
		CONTROLS = SHORT_COMMAND | 0x3,

		// Configure
		CALIBRATE = 0x71,
		CALIBRATING = 0x78,
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
		RX_LEVEL = 0x38,
		STABILIZER_FREQUENCY = 0x39,

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
		VIDEO_TAKE_PICTURE = 0xAA,
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
		VIDEO_WHITE_BALANCE = 0xC1,

		// Testing
		MOTOR_TEST = 0xD0,

		// User datas - 0x1xxx
		GET_USERNAME = 0x1001,

		// Errors - 0x7xxx
		// Hardware errors - 0x71xx
		// Host system errors - 0x72xx
		// Sanity errors - 0x7F003xx
		ERROR_DANGEROUS_BATTERY = 0x7301,
		// Camera/video errors - 0x7Fxx
		ERROR_CAMERA_MISSING = 0x7F01,
	} Cmd;

	Link* mLink;
	bool mConnected;
	bool mConnectionEstablished;
	uint32_t mLockState;

	static std::map< Cmd, std::string > mCommandsNames;
};

#endif // CONTROLLERBASE_H
