#include "LinuxCamera.h"
#include "Debug.h"

LinuxCamera::LinuxCamera()
{
	mCameraManager = std::make_unique<libcamera::CameraManager>();
	mCameraManager->start();

	if ( mCameraManager->cameras().empty() ) {
		gError() << "No camera found on this system";
		mCameraManager->stop();
		return;
	}

	gDebug() << "libcamera available cameras :";
	for ( auto const& camera : mCameraManager->cameras() ) {
		const libcamera::ControlList &props = camera->properties();
		std::string name;
		const auto &location = props.get(libcamera::properties::Location);
		if ( location ) {
			switch ( *location ) {
				case libcamera::properties::CameraLocationFront:
					name = "Internal front camera";
					break;
				case libcamera::properties::CameraLocationBack:
					name = "Internal back camera";
					break;
				case libcamera::properties::CameraLocationExternal:
					name = "External camera";
					const auto &model = props.get(libcamera::properties::Model);
					if ( model ) {
						name = " '" + *model + "'";
					}
					break;
			}
		}
		gDebug() << "    " << name << " (" << camera->id() << ")";
	}


	mCamera = mCameraManager->get( mCameraManager->cameras()[0]->id() );
	mCamera->acquire();


	mCameraConfiguration = mCamera->generateConfiguration( { libcamera::StreamRole::Raw, libcamera::StreamRole::Viewfinder, libcamera::StreamRole::VideoRecording, libcamera::StreamRole::StillCapture } );
	mRawStreamConfiguration = &mCameraConfiguration->at(0);
	mPreviewStreamConfiguration = &mCameraConfiguration->at(1);
	mVideoStreamConfiguration = &mCameraConfiguration->at(2);
	mStillStreamConfiguration = &mCameraConfiguration->at(3);

	for ( auto conf : {mPreviewStreamConfiguration, mVideoStreamConfiguration, mStillStreamConfiguration} ) {
		conf->pixelFormat = libcamera::formats::YUV420;
		conf->size.width = mWidth;
		conf->size.height = mHeight;
		conf->bufferCount = 2;
	}
	if ( dynamic_cast<LiveOutput>(mLiveEncoder) != nullptr ) {
		// TODO : use this condition instead of 'live_preview' modifier
		mPreviewStreamConfiguration->bufferCount = 1;
	}

	mCameraConfiguration->validate();
	gDebug() << "Validated viewfinder configuration is: " << mPreviewStreamConfiguration->toString();
	gDebug() << "Validated video configuration is: " << mVideoStreamConfiguration->toString();
	gDebug() << "Validated still configuration is: " << mStillStreamConfiguration->toString();

	mCamera->configure( mCameraConfiguration.get() );
}


LinuxCamera::~LinuxCamera()
{
}


void LinuxCamera::Start()
{
}


void LinuxCamera::Pause()
{
}


void LinuxCamera::Resume()
{
}


void LinuxCamera::getLensShader( Camera::LensShaderColor* r, Camera::LensShaderColor* g, Camera::LensShaderColor* b )
{
}


void LinuxCamera::setLensShader( const Camera::LensShaderColor& r, const Camera::LensShaderColor& g, const Camera::LensShaderColor& b )
{
}


std::string LinuxCamera::switchExposureMode()
{
}


std::string LinuxCamera::switchWhiteBalance()
{
}


std::string LinuxCamera::lockWhiteBalance()
{
}


void LinuxCamera::setNightMode( bool night_mode )
{
}


void LinuxCamera::setShutterSpeed( uint32_t value )
{
}


void LinuxCamera::setISO( int32_t value )
{
}


void LinuxCamera::setSaturation( int32_t value )
{
}


void LinuxCamera::setContrast( int32_t value )
{
}


void LinuxCamera::setBrightness( uint32_t value )
{
}


const bool LinuxCamera::recording()
{
}


const std::string LinuxCamera::exposureMode()
{
}


const std::string LinuxCamera::whiteBalance()
{
}


const bool LinuxCamera::nightMode()
{
}


const uint32_t LinuxCamera::shutterSpeed()
{
}


const int32_t LinuxCamera::ISO()
{
}


const int32_t LinuxCamera::saturation()
{
}


const int32_t LinuxCamera::contrast()
{
}


const uint32_t LinuxCamera::brightness()
{
}


const uint32_t LinuxCamera::framerate()
{
}


void LinuxCamera::TakePicture()
{
}


void LinuxCamera::StopRecording()
{
}


void LinuxCamera::StartRecording()
{
}


uint32_t LinuxCamera::getLastPictureID()
{
}


const std::string LinuxCamera::recordFilename()
{
}


uint32_t* LinuxCamera::getFileSnapshot( const std::string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp )
{
}
