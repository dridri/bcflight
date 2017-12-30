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
#include <map>
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

	typedef struct {
		uint8_t base;
		uint8_t radius;
		int8_t strength;
	} CameraLensShaderColor;

protected:
#define STATUS_ARMED		0b00000001
#define STATUS_CALIBRATED	0b00000010
#define STATUS_CALIBRATING	0b00000100
#define STATUS_NIGHTMODE	0b10000000

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

// Short commands only have 1 byte to define command ID
#define SHORT_COMMAND 0x80

	typedef enum {
		UNKNOWN = 0,

		// Continuous data (should be as small as possible)
		STATUS = SHORT_COMMAND | 0x0,
		PING = SHORT_COMMAND | 0x1,
		TELEMETRY = SHORT_COMMAND | 0x2,
		CONTROLS = SHORT_COMMAND | 0x3,

		// Special
		ACK_ID = 0x0600,

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
		GET_BOARD_INFOS = 0x90,
		GET_SENSORS_INFOS = 0x91,
		GET_CONFIG_FILE = 0x92,
		SET_CONFIG_FILE = 0x93,
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
		VIDEO_TAKE_PICTURE = 0xA2,
		VIDEO_START_RECORD = 0xA3,
		VIDEO_STOP_RECORD = 0xA4,
		GET_RECORDINGS_LIST = 0xA5,
		RECORD_DOWNLOAD = 0xA6,
		RECORD_DOWNLOAD_INIT = 0xA7,
		RECORD_DOWNLOAD_DATA = 0xA8,
		RECORD_DOWNLOAD_PROCESS = 0xA9,
		VIDEO_NIGHT_MODE = 0xAA,
		// Video settings
		VIDEO_BRIGHTNESS_INCR = 0xB1,
		VIDEO_BRIGHTNESS_DECR = 0xB2,
		VIDEO_CONTRAST_INCR = 0xB3,
		VIDEO_CONTRAST_DECR = 0xB4,
		VIDEO_SATURATION_INCR = 0xB5,
		VIDEO_SATURATION_DECR = 0xB6,
		VIDEO_ISO_INCR = 0xB7,
		VIDEO_ISO_DECR = 0xB8,
		VIDEO_SHUTTER_SPEED_INCR = 0xB9,
		VIDEO_SHUTTER_SPEED_DECR = 0xBA,
		VIDEO_ISO = 0xBB,
		VIDEO_SHUTTER_SPEED = 0xBC,
		VIDEO_WHITE_BALANCE = 0xC1,
		VIDEO_EXPOSURE_MODE = 0xC2,
		VIDEO_LOCK_WHITE_BALANCE = 0xC3,
		VIDEO_LENS_SHADER = 0xC5,
		VIDEO_SET_LENS_SHADER = 0xC6,

		// Testing
		MOTOR_TEST = 0xD0,

		// User datas - 0x1xxx
		GET_USERNAME = 0x1001,

		// Errors - 0x7xxx
		// Hardware errors - 0x71xx
		// Host system errors - 0x72xx
		// Sanity errors - 0x7F003xx
		ERROR_DANGEROUS_BATTERY = 0x7301,
		// Video/audio errors - 0x7Fxx
		ERROR_CAMERA_MISSING = 0x7F01,
		ERROR_MICROPHONE_MISSING = 0x7F02,
	} Cmd;

	Link* mLink;
	bool mConnected;
	bool mConnectionEstablished;
	uint32_t mLockState;
	uint16_t mTXAckID;
	uint16_t mRXAckID;

	static std::map< Cmd, std::string > mCommandsNames;
};

#endif // CONTROLLERBASE_H
