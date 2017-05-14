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

#include <unistd.h>
#include <mutex>
#include <list>

#include "links/Link.h"
#include "Thread.h"
#include "ControllerBase.h"

#define DECL_RO_VAR( type, n1, n2 ) \
	public: const type& n2() const { return m##n1; } \
	protected: type m##n1; \
	public:

#define DECL_RW_VAR( type, n1, n2 ) \
	public: const type& n2() const { return m##n1; } \
	protected: type m##n1; \
	public: void set##n1( const type& v );

typedef struct vec3 {
	float x, y, z;
} vec3;
typedef struct vec4 {
	float x, y, z, w;
} vec4;

class Controller : public ControllerBase, public ::Thread
{
public:
	typedef enum {
		Rate = 0,
		Stabilize = 1,
		ReturnToHome = 2,
		Follow = 3,
	} Mode;

	Controller( Link* link, bool spectate = false );
	virtual ~Controller();
	bool isSpectate() const { return mSpectate; }

	void Calibrate();
	void CalibrateAll();
	void CalibrateESCs();
	void Arm();
	void Disarm();
	void ResetBattery();
	void setFullTelemetry( bool fullt );
	std::string getBoardInfos();
	std::string getSensorsInfos();
	std::string debugOutput();
	std::vector< std::string > recordingsList();

	std::string getConfigFile();
	void setConfigFile( const std::string& content );
	void UploadUpdateInit();
	void UploadUpdateData( const uint8_t* buf, uint32_t offset, uint32_t size );
	void UploadUpdateProcess( const uint8_t* buf, uint32_t size );
	void EnableTunDevice();
	void DisableTunDevice();

	void setRoll( const float& v );
	void setPitch( const float& v );
	void setYaw( const float& v );
	void setThrustRelative( const float& v );
	void ReloadPIDs();

	void VideoPause();
	void VideoResume();
	void VideoTakePicture();
	void VideoBrightnessIncrease();
	void VideoBrightnessDecrease();
	void VideoContrastIncrease();
	void VideoContrastDecrease();
	void VideoSaturationIncrease();
	void VideoSaturationDecrease();
	std::string VideoWhiteBalance();

	DECL_RO_VAR( uint32_t, Ping, ping );
	DECL_RO_VAR( bool, Calibrated, calibrated );
	DECL_RO_VAR( bool, Calibrating, calibrating );
	DECL_RO_VAR( bool, Armed, armed );
	DECL_RO_VAR( uint32_t, TotalCurrent, totalCurrent );
	DECL_RO_VAR( float, CurrentDraw, currentDraw );
	DECL_RO_VAR( float, BatteryVoltage, batteryVoltage );
	DECL_RO_VAR( float, BatteryLevel, batteryLevel );
	DECL_RO_VAR( uint32_t, CPULoad, CPULoad );
	DECL_RO_VAR( uint32_t, CPUTemp, CPUTemp );
	DECL_RO_VAR( float, Altitude, altitude );
	DECL_RW_VAR( vec3, RollPID, rollPid );
	DECL_RW_VAR( vec3, PitchPID, pitchPid );
	DECL_RW_VAR( vec3, YawPID, yawPid );
	DECL_RW_VAR( vec3, OuterPID, outerPid );
	DECL_RW_VAR( vec3, HorizonOffset, horizonOffset );
	DECL_RW_VAR( float, Thrust, thrust );
	DECL_RW_VAR( vec3, RPY, rpy );
	DECL_RW_VAR( vec3, ControlRPY, controlRPY );
	DECL_RW_VAR( Mode, Mode, mode );
	DECL_RO_VAR( bool, PIDsLoaded, PIDsLoaded );
	DECL_RO_VAR( uint32_t, DroneRxQuality, droneRxQuality );
	DECL_RO_VAR( int32_t, DroneRxLevel, droneRxLevel );
	DECL_RW_VAR( bool, NightMode, nightMode );
	DECL_RO_VAR( uint32_t, StabilizerFrequency, stabilizerFrequency );
	DECL_RO_VAR( std::vector<float>, MotorsSpeed, motorsSpeed );

	DECL_RO_VAR( std::string, Username, username );

	void MotorTest(uint32_t id);
	
	// Errors
	DECL_RO_VAR( bool, CameraMissing, cameraMissing );

	float acceleration() const;
	std::list< vec4 > rpyHistory();
	std::list< vec4 > ratesHistory();
	std::list< vec3 > outerPidHistory();
	std::list< float > altitudeHistory();

	float localBatteryVoltage() const;
	virtual uint16_t rawThrust() { return 0; }
	virtual uint16_t rawYaw() { return 0; }
	virtual uint16_t rawRoll() { return 0; }
	virtual uint16_t rawPitch() { return 0; }
	virtual void SaveThrustCalibration( uint16_t min, uint16_t center, uint16_t max ) {}
	virtual void SaveYawCalibration( uint16_t min, uint16_t center, uint16_t max ) {}
	virtual void SavePitchCalibration( uint16_t min, uint16_t center, uint16_t max ) {}
	virtual void SaveRollCalibration( uint16_t min, uint16_t center, uint16_t max ) {}

protected:
	virtual float ReadThrust() { return 0.0f; }
	virtual float ReadRoll() { return 0.0f; }
	virtual float ReadPitch() { return 0.0f; }
	virtual float ReadYaw() { return 0.0f; }
	virtual int8_t ReadSwitch( uint32_t id ) { return 0; }
	virtual bool run();
	bool RxRun();
	uint32_t crc32( const uint8_t* buf, uint32_t len );

	bool mSpectate;
	Packet mTxFrame;
	std::mutex mXferMutex;
	uint64_t mTickBase;
	uint32_t mUpdateTick;
	uint64_t mUpdateCounter;
	uint64_t mPingTimer;
	uint64_t mDataTimer;
	uint64_t mMsCounter;
	uint64_t mMsCounter50;
	bool mRequestAck;

	Controls mControls;

	HookThread<Controller>* mRxThread;
	std::string mBoardInfos;
	std::string mSensorsInfos;
	std::string mConfigFile;
	std::string mRecordingsList;
	bool mUpdateUploadValid;
	bool mConfigUploadValid;

	uint32_t mTicks;
	uint32_t mSwitches[8];
	bool mVideoRecording;
	std::string mVideoWhiteBalance;

	float mAcceleration;
	std::list< vec4 > mRPYHistory;
	std::list< vec4 > mRatesHistory;
	std::list< vec3 > mOuterPIDHistory;
	std::list< float > mAltitudeHistory;
	std::mutex mHistoryMutex;

	float mLocalBatteryVoltage;
	std::string mDebug;
	std::mutex mDebugMutex;
};

#endif // CONTROLLER_H
