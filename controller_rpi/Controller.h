#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <mutex>
#include <gammaengine/Thread.h>
#include <gammaengine/Input.h>
#include <gammaengine/Timer.h>
#include <gammaengine/Socket.h>
#include <gammaengine/Renderer2D.h>
#include <gammaengine/Font.h>

#include "ADCs/MCP320x.h" // TEST

using namespace GE;
// class Settings;

#define DECL_RO_VAR( type, n1, n2 ) \
	public: const type& n2() const { return m##n1; } \
	protected: type m##n1;

#define DECL_RW_VAR( type, n1, n2 ) \
	public: const type& n2() const { return m##n1; } \
	protected: type m##n1; \
	public: void set##n1( const type& v );

class Controller : protected Thread
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	class Joystick {
	public:
		Joystick() : mFd( -1 ), mCalibrated( false ), mThrustMode( false ), mMin( 0 ), mCenter( 0 ), mMax( 0 ) {}
		Joystick( int id, const std::string& devfile, bool thrust_mode = false );
		~Joystick();
		uint16_t ReadRaw();
		float Read();
		void SetCalibratedValues( uint16_t min, uint16_t center, uint16_t max );
	private:
		int mId;
		std::string mDevFile;
		int mFd;
		bool mCalibrated;
		bool mThrustMode;
		uint16_t mMin;
		uint16_t mCenter;
		uint16_t mMax;
	};

	Controller( const std::string& addr, uint16_t port );
	~Controller();

	Joystick* joystick( int x ) { return &mJoysticks[x]; }

	void Arm();
	void Disarm();
	void ResetBattery();
	void setRoll( const float& v );
	void setPitch( const float& v );
	void setYaw( const float& v );
	void setThrustRelative( const float& v );

	DECL_RO_VAR( uint32_t, Ping, ping );
	DECL_RO_VAR( bool, Armed, armed );
	DECL_RO_VAR( uint32_t, TotalCurrent, totalCurrent );
	DECL_RO_VAR( float, CurrentDraw, currentDraw );
	DECL_RO_VAR( float, BatteryVoltage, batteryVoltage );
	DECL_RO_VAR( float, BatteryLevel, batteryLevel );
	DECL_RW_VAR( Vector3f, PID, pid );
	DECL_RW_VAR( Vector3f, OuterPID, outerPid );
	DECL_RW_VAR( float, Thrust, thrust );
	DECL_RW_VAR( Vector3f, RPY, rpy );
	DECL_RW_VAR( Vector3f, ControlRPY, controlRPY );
	DECL_RW_VAR( Mode, Mode, mode );

	const std::list< Vector3f >& rpyHistory() const;
	const std::list< Vector3f >& outerPidHistory() const;

protected:
	typedef enum {
		// Configure
		PING = 0x70,
		CALIBRATE = 0x71,
		SET_TIMESTAMP = 0x72,
		ARM = 0x73,
		DISARM = 0x74,
		RESET_BATTERY = 0x75,
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
		RESET_MOTORS = 0x47,
		SET_MODE = 0x48,
		SET_PID_P = 0x50,
		SET_PID_I = 0x51,
		SET_PID_D = 0x52,
		SET_OUTER_PID_P = 0x53,
		SET_OUTER_PID_I = 0x54,
		SET_OUTER_PID_D = 0x55,
	} Cmd;

	virtual bool run();

	void Send( const Cmd& cmd );
	void Send( const Cmd& cmd, uint32_t v );
	void Send( const Cmd& cmd, float v );
	uint32_t ReceiveU32();
	float ReceiveFloat();

// 	Settings* mSettings;
	Socket* mSocket;
	std::mutex mXferMutex;
	uint32_t mUpdateTick;
	uint64_t mUpdateCounter;
	Timer mPingTimer;

	uint32_t mTicks;
	Joystick mJoysticks[8];

	MCP320x* mADC;

	std::list< Vector3f > mRPYHistory;
	std::list< Vector3f > mOuterPIDHistory;
};

#endif // CONTROLLER_H
