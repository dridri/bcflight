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

typedef struct vec2 {
	float x, y, z;
} vec2;
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
	void setUpdateFrequency( uint32_t freq_hz );

	void Calibrate();
	void CalibrateAll();
	void CalibrateESCs();
	void Arm();
	void Disarm();
	void ResetBattery();
	void setFullTelemetry( bool fullt );
	string getBoardInfos();
	string getSensorsInfos();
	string getCamerasInfos();
	string debugOutput();
	vector< string > recordingsList();

	string getConfigFile();
	void setConfigFile( const string& content );
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
	string VideoWhiteBalance();
	string VideoLockWhiteBalance();
	string VideoExposureMode();
	int32_t VideoGetIso();
	uint32_t VideoGetShutterSpeed();
	void VideoIsoIncrease();
	void VideoIsoDecrease();
	void VideoShutterSpeedIncrease();
	void VideoShutterSpeedDecrease();
	void getCameraLensShader( CameraLensShaderColor* r, CameraLensShaderColor* g, CameraLensShaderColor* b );
	void setCameraLensShader( const CameraLensShaderColor& r, const CameraLensShaderColor& g, const CameraLensShaderColor& b );

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
	DECL_RO_VAR( vector<float>, MotorsSpeed, motorsSpeed );

	DECL_RO_VAR( string, Username, username );

	void MotorTest(uint32_t id);
	
	// Errors
	DECL_RO_VAR( bool, CameraMissing, cameraMissing );

	float acceleration() const;
	list< vec4 > rpyHistory();
	list< vec4 > ratesHistory();
	list< vec4 > ratesDerivativeHistory();
	list< vec4 > accelerationHistory();
	list< vec4 > magnetometerHistory();
	list< vec3 > outerPidHistory();
	list< vec2 > altitudeHistory();

	float localBatteryVoltage() const;
	virtual uint16_t rawThrust( float dt ) { return 0; }
	virtual uint16_t rawYaw( float dt ) { return 0; }
	virtual uint16_t rawRoll( float dt ) { return 0; }
	virtual uint16_t rawPitch( float dt ) { return 0; }
	virtual void SaveThrustCalibration( uint16_t min, uint16_t center, uint16_t max ) {}
	virtual void SaveYawCalibration( uint16_t min, uint16_t center, uint16_t max ) {}
	virtual void SavePitchCalibration( uint16_t min, uint16_t center, uint16_t max ) {}
	virtual void SaveRollCalibration( uint16_t min, uint16_t center, uint16_t max ) {}
	virtual bool SimulatorMode( bool enabled ) { return false; }

protected:
	virtual float ReadThrust( float dt ) { return 0.0f; }
	virtual float ReadRoll( float dt ) { return 0.0f; }
	virtual float ReadPitch( float dt ) { return 0.0f; }
	virtual float ReadYaw( float dt ) { return 0.0f; }
	virtual int8_t ReadSwitch( uint32_t id ) { return 0; }
	virtual bool run();
	bool RxRun();
	uint32_t crc32( const uint8_t* buf, uint32_t len );

	uint32_t mUpdateFrequency;
	bool mSpectate;
	Packet mTxFrame;
	mutex mXferMutex;
	uint64_t mTickBase;
	uint32_t mUpdateTick;
	uint64_t mUpdateCounter;
	uint64_t mPingTimer;
	uint64_t mTelemetryTimer;
	uint64_t mDataTimer;
	uint64_t mMsCounter;
	uint64_t mMsCounter50;
	bool mRequestAck;

	Controls mControls;

	HookThread<Controller>* mRxThread;
	string mBoardInfos;
	string mSensorsInfos;
	string mCamerasInfos;
	string mConfigFile;
	string mRecordingsList;
	bool mUpdateUploadValid;
	bool mConfigUploadValid;

	uint32_t mTicks;
	uint32_t mSwitches[12];
	bool mVideoRecording;
	string mVideoWhiteBalance;
	string mVideoExposureMode;
	int32_t mVideoIso;
	int32_t mVideoShutterSpeed;
	CameraLensShaderColor mCameraLensShaderR;
	CameraLensShaderColor mCameraLensShaderG;
	CameraLensShaderColor mCameraLensShaderB;

	float mAcceleration;
	list< vec4 > mRPYHistory;
	list< vec4 > mRatesHistory;
	list< vec4 > mRatesDerivativeHistory;
	list< vec4 > mAccelerationHistory;
	list< vec4 > mMagnetometerHistory;
	list< vec3 > mOuterPIDHistory;
	list< vec2 > mAltitudeHistory;
	mutex mHistoryMutex;

	float mLocalBatteryVoltage;
	string mDebug;
	mutex mDebugMutex;
};

#endif // CONTROLLER_H
