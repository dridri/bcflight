#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <unistd.h>
#include <mutex>
#include <list>

#include "links/Link.h"
#include "Thread.h"
// #include "ADCs/MCP320x.h" // TEST

#define DECL_RO_VAR( type, n1, n2 ) \
	public: const type& n2() const { return m##n1; } \
	protected: type m##n1;

#define DECL_RW_VAR( type, n1, n2 ) \
	public: const type& n2() const { return m##n1; } \
	protected: type m##n1; \
	public: void set##n1( const type& v );

typedef struct vec3 {
	float x, y, z;
} vec3;

class Controller : protected ::Thread
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;
/*
	class Joystick {
	public:
		Joystick() : mADC( nullptr ), mADCChannel( 0 ), mCalibrated( false ), mThrustMode( false ), mMin( 0 ), mCenter( 0 ), mMax( 0 ) {}
		Joystick( MCP320x* adc, int id, int channel, bool thrust_mode = false );
		~Joystick();
		uint16_t ReadRaw();
		float Read();
		void SetCalibratedValues( uint16_t min, uint16_t center, uint16_t max );
		uint16_t max() const { return mMax; }
		uint16_t center() const { return mCenter; }
		uint16_t min() const { return mMin; }
	private:
		MCP320x* mADC;
		int mId;
		int mADCChannel;
		bool mCalibrated;
		bool mThrustMode;
		uint16_t mMin;
		uint16_t mCenter;
		uint16_t mMax;
	};
*/
	Controller( Link* link );
	virtual ~Controller();
	bool isConnected() const { return mLink->isConnected(); }
	Link* link() const { return mLink; }

	void Lock() { mLockState = 1; while ( mLockState != 2 ) usleep(1); }
	void Unlock() { mLockState = 0; }
// 	Joystick* joystick( int x ) { return &mJoysticks[x]; }

	void Calibrate();
	void CalibrateAll();
	void CalibrateESCs();
	void Arm();
	void Disarm();
	void ResetBattery();
	void setFullTelemetry( bool fullt );
	std::string getBoardInfos();
	std::string getSensorsInfos();

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
	DECL_RO_VAR( uint32_t, CPULoad, CPULoad );
	DECL_RO_VAR( uint32_t, CPUTemp, CPUTemp );
	DECL_RO_VAR( float, Altitude, altitude );
	DECL_RW_VAR( vec3, PID, pid );
	DECL_RW_VAR( vec3, OuterPID, outerPid );
	DECL_RW_VAR( vec3, HorizonOffset, horizonOffset );
	DECL_RW_VAR( float, Thrust, thrust );
	DECL_RW_VAR( vec3, RPY, rpy );
	DECL_RW_VAR( vec3, ControlRPY, controlRPY );
	DECL_RW_VAR( Mode, Mode, mode );

	float acceleration() const;
	const std::list< vec3 >& rpyHistory() const;
	const std::list< vec3 >& outerPidHistory() const;
	const std::list< float >& altitudeHistory() const;

	float localBatteryVoltage() const;

protected:
	typedef enum {
		UNKNOWN = 0x0,
		// Configure
		PING = 0x70,
		CALIBRATE = 0x71,
		SET_TIMESTAMP = 0x72,
		ARM = 0x73,
		DISARM = 0x74,
		RESET_BATTERY = 0x75,
		CALIBRATE_ESCS = 0x76,
		SET_FULL_TELEMETRY = 0x77,
		GET_BOARD_INFOS = 0x80,
		GET_SENSORS_INFOS = 0x81,
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
		HORIZON_OFFSET = 0x25,
		VBAT = 0x30,
		TOTAL_CURRENT = 0x31,
		CURRENT_DRAW = 0x32,
		BATTERY_LEVEL = 0x34,
		CPU_LOAD = 0x35,
		CPU_TEMP = 0x36,
		// Setters
		SET_ROLL = 0x40,
		SET_PITCH = 0x41,
		SET_YAW = 0x42,
		SET_THRUST = 0x43,
		RESET_MOTORS = 0x47,
		SET_MODE = 0x48,
		SET_ALTITUDE_HOLD = 0x49,
		SET_PID_P = 0x50,
		SET_PID_I = 0x51,
		SET_PID_D = 0x52,
		SET_OUTER_PID_P = 0x53,
		SET_OUTER_PID_I = 0x54,
		SET_OUTER_PID_D = 0x55,
		SET_HORIZON_OFFSET = 0x56,
		// Video
		VIDEO_START_RECORD = 0xA0,
		VIDEO_STOP_RECORD = 0xA1,
		VIDEO_GET_BRIGHTNESS = 0xA2,
		VIDEO_SET_BRIGHTNESS = 0xAA,
	} Cmd;

	virtual float ReadThrust() = 0;
	virtual float ReadRoll() = 0;
	virtual float ReadPitch() = 0;
	virtual float ReadYaw() = 0;
	virtual int8_t ReadSwitch( uint32_t id ) = 0;
	virtual bool run();
	bool RxRun();

	Link* mLink;
	Packet mTxFrame;
	bool mConnected;
	uint32_t mLockState;
	std::mutex mXferMutex;
	uint32_t mUpdateTick;
	uint64_t mUpdateCounter;
	uint64_t mPingTimer;
	uint64_t mMsCounter;
	uint64_t mMsCounter50;

	HookThread<Controller>* mRxThread;
	std::string mBoardInfos;
	std::string mSensorsInfos;

	uint32_t mTicks;
// 	Joystick mJoysticks[8];
	bool mSwitches[5];

// 	MCP320x* mADC;

	float mAcceleration;
	std::list< vec3 > mRPYHistory;
	std::list< vec3 > mOuterPIDHistory;
	std::list< float > mAltitudeHistory;

	float mLocalBatteryVoltage;

	static std::map< Cmd, std::string > mCommandsNames;
};

#endif // CONTROLLER_H
