#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <mutex>
#include <Thread.h>
#include "Vector.h"
#include "EKFSmoother.h"

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
		PID_FACTORS = 0x23,
		OUTER_PID_FACTORS = 0x24,
		VBAT = 0x30,
		TOTAL_CURRENT = 0x31,
		CURRENT_DRAW = 0x32,
		BATTERY_LEVEL = 0x34,
		// Setters
		SET_ROLL = 0x40,
		SET_PITCH = 0x41,
		SET_YAW = 0x42,
		SET_THRUST = 0x43,
		SET_TRPY = 0x44,
		RESET_MOTORS = 0x47,
		SET_MODE = 0x48,
		SET_PID_P = 0x50,
		SET_PID_I = 0x51,
		SET_PID_D = 0x52,
		SET_OUTER_PID_P = 0x53,
		SET_OUTER_PID_I = 0x54,
		SET_OUTER_PID_D = 0x55,
		// Video
		VIDEO_START_RECORD = 0xA0,
		VIDEO_STOP_RECORD = 0xA1,
		VIDEO_GET_BRIGHTNESS = 0xA2,
		VIDEO_SET_BRIGHTNESS = 0xAA,
	} Cmd;

	virtual bool run();
	bool TelemetryRun();

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
};

#endif // CONTROLLER_H
