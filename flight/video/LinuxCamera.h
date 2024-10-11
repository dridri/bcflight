#ifndef LINUXCAMERA_H
#define LINUXCAMERA_H

#include <libcamera/libcamera.h>
#include "DRMFrameBuffer.h"
#include "DRMSurface.h"
#include <Thread.h>
#include "Camera.h"
#include "Lua.h"
#include <set>

LUA_CLASS class LinuxCamera : public Camera
{
public:
	LUA_EXPORT LinuxCamera();
	virtual ~LinuxCamera();

	virtual void Start();
	void Stop();
	virtual void Pause();
	virtual void Resume();
	virtual void StartRecording();
	virtual void StopRecording();
	virtual void TakePicture();

	virtual const uint32_t width();
	virtual const uint32_t height();
	virtual const uint32_t framerate();
	virtual const float brightness();
	virtual const float contrast();
	virtual const float saturation();
	virtual const int32_t ISO();
	virtual const uint32_t shutterSpeed();
	virtual const bool nightMode();
	virtual const string whiteBalance();
	virtual const string exposureMode();
	virtual const bool recording();
	virtual const string recordFilename();
	LUA_EXPORT virtual void setBrightness( float value );
	LUA_EXPORT virtual void setContrast( float value );
	LUA_EXPORT virtual void setSaturation( float value );
	LUA_EXPORT virtual void setISO( int32_t value );
	virtual void setShutterSpeed( uint32_t value );
	// virtual void setNightMode( bool night_mode );
	LUA_EXPORT virtual void updateSettings();
	virtual void setHDR( bool hdr, bool force = false );
	virtual string switchWhiteBalance();
	virtual string lockWhiteBalance();
	virtual string switchExposureMode();
	virtual void setLensShader( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B );
	virtual void getLensShader( LensShaderColor* r, LensShaderColor* g, LensShaderColor* b );
	virtual uint32_t getLastPictureID();

	virtual uint32_t* getFileSnapshot( const string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp );

	virtual LuaValue infos();

protected:
	bool mLivePreview;
//	_LUA_PROPERTY("sensor_mode") int32_t mSensorMode;
	LUA_PROPERTY("width") int32_t mWidth;
	LUA_PROPERTY("height") int32_t mHeight;
	LUA_PROPERTY("framerate") int32_t mFps;
	LUA_PROPERTY("hdr") bool mHDR;
//	_LUA_PROPERTY("exposure") int32_t mExposure;
//	_LUA_PROPERTY("iso") int32_t mIso;
	LUA_PROPERTY("shutter_speed") int32_t mShutterSpeed;
	LUA_PROPERTY("iso") uint32_t mISO;
//	_LUA_PROPERTY("analogue_gain") float mAnalogueGain;
//	_LUA_PROPERTY("digital_gain") float mDigitalGain;
	LUA_PROPERTY("sharpness") float mSharpness;
	LUA_PROPERTY("brightness") float mBrightness;
	LUA_PROPERTY("contrast") float mContrast;
	LUA_PROPERTY("saturation") float mSaturation;
	LUA_PROPERTY("vflip") bool mVflip;
	LUA_PROPERTY("hflip") bool mHflip;
	LUA_PROPERTY("white_balance") std::string mWhiteBalance;
	LUA_PROPERTY("exposure") std::string mExposureMode;
//	_LUA_PROPERTY("stabilisation") bool mStabilisation;
//	_LUA_PROPERTY("night_fps") int32_t mNightFps;
	LUA_PROPERTY("night_iso") uint32_t mNightISO;
	LUA_PROPERTY("night_brightness") float mNightBrightness;
	LUA_PROPERTY("night_contrast") float mNightContrast;
	LUA_PROPERTY("night_saturation") float mNightSaturation;

	int64_t mCurrentFramerate;
	int64_t mExposureTime;
	libcamera::Span<const float, 2> mAwbGains;
	int32_t mColorTemperature;
	bool mNightMode;

	static std::unique_ptr<libcamera::CameraManager> sCameraManager;
	std::shared_ptr<libcamera::Camera> mCamera;
	std::unique_ptr<libcamera::CameraConfiguration> mCameraConfiguration;
	std::vector<std::unique_ptr<libcamera::Request>> mRequests;
	std::set<const libcamera::Request*> mRequestsAlive;
	std::mutex mRequestsAliveMutex;
	libcamera::StreamConfiguration* mRawStreamConfiguration;
	libcamera::StreamConfiguration* mPreviewStreamConfiguration;
	libcamera::StreamConfiguration* mVideoStreamConfiguration;
	libcamera::StreamConfiguration* mStillStreamConfiguration;
	std::map< const libcamera::Stream*, libcamera::ControlList > mControlListQueue;
	std::mutex mControlListQueueMutex;
	libcamera::FrameBufferAllocator* mAllocator;
	libcamera::ControlList mAllControls;

	std::map<libcamera::FrameBuffer*, uint8_t* > mPlaneBuffers;
	std::map<libcamera::FrameBuffer*, DRMFrameBuffer*> mPreviewFrameBuffers;
	DRMSurface* mPreviewSurface;
	bool mPreviewSurfaceSet;

	HookThread<LinuxCamera>* mLiveThread;
	bool mStopping;

 	bool setWhiteBalance( const std::string& mode, libcamera::ControlList* controlsList = nullptr );

	void requestComplete( libcamera::Request* request );
	void pushControlList( const libcamera::ControlList& list );
	bool LiveThreadRun();
	bool RecordThreadRun();
	bool TakePictureThreadRun();
};

#endif // LINUXCAMERA_H
