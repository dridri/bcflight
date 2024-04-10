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

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <math.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>
#include <stdio.h>
#include "ControllerBase.h"

map< ControllerBase::Cmd, string > ControllerBase::mCommandsNames = {
	{ ControllerBase::UNKNOWN, "Unknown" },
	// Configure
	{ ControllerBase::STATUS, "Status" },
	{ ControllerBase::PING, "Ping" },
	{ ControllerBase::TELEMETRY, "Telemetry" },
	{ ControllerBase::CONTROLS, "Controls" },
	{ ControllerBase::CALIBRATE, "Calibrate" },
	{ ControllerBase::SET_TIMESTAMP, "Set timestamp" },
	{ ControllerBase::ARM, "Arm" },
	{ ControllerBase::DISARM, "Disarm" },
	{ ControllerBase::RESET_BATTERY, "Reset battery" },
	{ ControllerBase::CALIBRATE_ESCS, "Calibrate ESCs" },
	{ ControllerBase::SET_FULL_TELEMETRY, "0x77" },
	{ ControllerBase::DEBUG_OUTPUT, "0x7A" },
	{ ControllerBase::GET_BOARD_INFOS, "Board infos" },
	{ ControllerBase::GET_SENSORS_INFOS, "Sensors infos" },
	{ ControllerBase::GET_CONFIG_FILE, "Get config file" },
	{ ControllerBase::SET_CONFIG_FILE, "Set config file" },
	{ ControllerBase::UPDATE_UPLOAD_INIT, "0x9A" },
	{ ControllerBase::UPDATE_UPLOAD_DATA, "0x9B" },
	{ ControllerBase::UPDATE_UPLOAD_PROCESS, "0x9C" },
	// Getters
	{ ControllerBase::PRESSURE, "Pressure" },
	{ ControllerBase::TEMPERATURE, "Temperature" },
	{ ControllerBase::ALTITUDE, "Altitude" },
	{ ControllerBase::ROLL, "Roll" },
	{ ControllerBase::PITCH, "Pitch" },
	{ ControllerBase::YAW, "Yaw" },
	{ ControllerBase::ROLL_PITCH_YAW, "Roll-Pitch-Yaw" },
	{ ControllerBase::ACCEL, "Acceleration" },
	{ ControllerBase::GYRO, "Gyroscope" },
	{ ControllerBase::MAGN, "Magnetometer" },
	{ ControllerBase::MOTORS_SPEED, "Motors speed" },
	{ ControllerBase::CURRENT_ACCELERATION, "Current acceleration" },
	{ ControllerBase::SENSORS_DATA, "Sensors data" },
	{ ControllerBase::PID_OUTPUT, "PID output" },
	{ ControllerBase::OUTER_PID_OUTPUT, "Outer PID output" },
	{ ControllerBase::ROLL_PID_FACTORS, "Roll PID factors" },
	{ ControllerBase::PITCH_PID_FACTORS, "Pitch PID factors" },
	{ ControllerBase::YAW_PID_FACTORS, "Yaw PID factors" },
	{ ControllerBase::OUTER_PID_FACTORS, "Outer PID factors" },
	{ ControllerBase::HORIZON_OFFSET, "Horizon offset" },
	{ ControllerBase::VBAT, "Battery voltage" },
	{ ControllerBase::TOTAL_CURRENT, "Total current draw" },
	{ ControllerBase::CURRENT_DRAW, "Current draw" },
	{ ControllerBase::BATTERY_LEVEL, "Battery level" },
	{ ControllerBase::CPU_LOAD, "CPU Load" },
	{ ControllerBase::CPU_TEMP, "CPU Temp" },
	{ ControllerBase::RX_QUALITY, "RX Quality" },
	// Setters
	{ ControllerBase::SET_ROLL, "Set roll" },
	{ ControllerBase::SET_PITCH, "Set pitch" },
	{ ControllerBase::SET_YAW, "Set yaw" },
	{ ControllerBase::SET_THRUST, "Set thrust" },
	{ ControllerBase::RESET_MOTORS, "Reset motors" },
	{ ControllerBase::SET_MODE, "Set mode" },
	{ ControllerBase::SET_ALTITUDE_HOLD, "Altitude hold" },
	{ ControllerBase::SET_ROLL_PID_P, "0x50" },
	{ ControllerBase::SET_ROLL_PID_I, "0x51" },
	{ ControllerBase::SET_ROLL_PID_D, "0x52" },
	{ ControllerBase::SET_PITCH_PID_P, "0x53" },
	{ ControllerBase::SET_PITCH_PID_I, "0x54" },
	{ ControllerBase::SET_PITCH_PID_D, "0x55" },
	{ ControllerBase::SET_YAW_PID_P, "0x56" },
	{ ControllerBase::SET_YAW_PID_I, "0x57" },
	{ ControllerBase::SET_YAW_PID_D, "0x58" },
	{ ControllerBase::SET_OUTER_PID_P, "0x59" },
	{ ControllerBase::SET_OUTER_PID_I, "0x5A" },
	{ ControllerBase::SET_OUTER_PID_D, "0x5B" },
	{ ControllerBase::SET_HORIZON_OFFSET, "Set horizon offset" },
	// Video
	{ ControllerBase::VIDEO_START_RECORD, "Start video record" },
	{ ControllerBase::VIDEO_STOP_RECORD, "Stop video record" },
	{ ControllerBase::VIDEO_BRIGHTNESS_INCR, "Increase video brightness" },
	{ ControllerBase::VIDEO_BRIGHTNESS_DECR, "Decrease video brightness" },
	{ ControllerBase::VIDEO_CONTRAST_INCR, "Increase video contrast" },
	{ ControllerBase::VIDEO_CONTRAST_DECR, "Decrease video contrast" },
	{ ControllerBase::VIDEO_SATURATION_INCR, "Increase video saturation" },
	{ ControllerBase::VIDEO_SATURATION_DECR, "Decrease video saturation" },
	{ ControllerBase::VIDEO_NIGHT_MODE, "Set video night mode" },
	{ ControllerBase::VIDEO_WHITE_BALANCE, "Switch video white balance" },
	{ ControllerBase::VIDEO_LOCK_WHITE_BALANCE, "Lock video white balance" },
	// User datas
	{ ControllerBase::GET_USERNAME, "Get username" },
	// Testing
	{ ControllerBase::MOTOR_TEST, "Motor test" },
	{ ControllerBase::MOTORS_BEEP, "Motors beep" },
};

ControllerBase::ControllerBase( Link* link )
	: mLink( link )
	, mConnected( false )
	, mConnectionEstablished( false )
	, mLockState( 0 )
	, mTXAckID( 0 )
	, mRXAckID( 0 )
{
}


ControllerBase::~ControllerBase()
{
}
