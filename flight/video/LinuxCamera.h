#ifndef LINUXCAMERA_H
#define LINUXCAMERA_H

#include <libcamera/libcamera.h>
#include "Camera.h"
#include "Lua.h"

LUA_CLASS class LinuxCamera : public Camera
{
public:
	LUA_EXPORT LinuxCamera();
	~LinuxCamera();

	virtual void Start();
	virtual void Pause();
	virtual void Resume();
	virtual void StartRecording();
	virtual void StopRecording();
	virtual void TakePicture();

	virtual const uint32_t framerate();
	virtual const uint32_t brightness();
	virtual const int32_t contrast();
	virtual const int32_t saturation();
	virtual const int32_t ISO();
	virtual const uint32_t shutterSpeed();
	virtual const bool nightMode();
	virtual const string whiteBalance();
	virtual const string exposureMode();
	virtual const bool recording();
	virtual const string recordFilename();
	virtual void setBrightness( uint32_t value );
	virtual void setContrast( int32_t value );
	virtual void setSaturation( int32_t value );
	virtual void setISO( int32_t value );
	virtual void setShutterSpeed( uint32_t value );
	virtual void setNightMode( bool night_mode );
	virtual string switchWhiteBalance();
	virtual string lockWhiteBalance();
	virtual string switchExposureMode();
	virtual void setLensShader( const LensShaderColor& R, const LensShaderColor& G, const LensShaderColor& B );
	virtual void getLensShader( LensShaderColor* r, LensShaderColor* g, LensShaderColor* b );
	virtual uint32_t getLastPictureID();

	virtual uint32_t* getFileSnapshot( const string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp );

protected:
	LUA_PROPERTY("sensor_mode") int32_t mSensorMode;
	LUA_PROPERTY("width") int32_t mWidth;
	LUA_PROPERTY("height") int32_t mHeight;
	LUA_PROPERTY("fps") int32_t mFps;
	LUA_PROPERTY("exposure") int32_t mExposure;
	LUA_PROPERTY("iso") int32_t mIso;
	LUA_PROPERTY("shutter_speed") int32_t mShutterSpeed;
	LUA_PROPERTY("sharpness") int32_t mSharpness;
	LUA_PROPERTY("brightness") int32_t mBrightness;
	LUA_PROPERTY("contrast") int32_t mContrast;
	LUA_PROPERTY("saturation") int32_t mSaturation;
	LUA_PROPERTY("vflip") bool mVflip;
	LUA_PROPERTY("hflip") bool mHflip;
	LUA_PROPERTY("whiteBalance") std::string mWhiteBalance;
	LUA_PROPERTY("stabilisation") bool mStabilisation;
	LUA_PROPERTY("night_fps") int32_t mNightFps;
	LUA_PROPERTY("night_iso") int32_t mNightIso;
	LUA_PROPERTY("night_brightness") int32_t mNightBrightness;
	LUA_PROPERTY("night_contrast") int32_t mNightContrast;
	LUA_PROPERTY("night_saturation") int32_t mNightSaturation;

	std::unique_ptr<libcamera::CameraManager> mCameraManager;
	std::shared_ptr<libcamera::Camera> mCamera;
	std::unique_ptr<libcamera::CameraConfiguration> mCameraConfiguration;
	libcamera::StreamConfiguration* mRawStreamConfiguration;
	libcamera::StreamConfiguration* mPreviewStreamConfiguration;
	libcamera::StreamConfiguration* mVideoStreamConfiguration;
	libcamera::StreamConfiguration* mStillStreamConfiguration;
};

#endif // LINUXCAMERA_H
