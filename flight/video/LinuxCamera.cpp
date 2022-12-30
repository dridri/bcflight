#include "LinuxCamera.h"
#include "LiveOutput.h"
#include "Debug.h"

std::unique_ptr<libcamera::CameraManager> LinuxCamera::sCameraManager = nullptr;



class LibCameraStream : private std::streambuf, public std::ostream {
public:
	LibCameraStream() : std::ostream(this) {}
	

private:
	int overflow( int c ) override {
		// printf("0x%02X, '%c'\n", c, c);
		if ( c == '\n' ) {
			int32_t pos = buf.find("]");
			if ( pos > 0 ) {
				pos = buf.find("]", pos + 1);
				if ( pos > 0 ) {
					int32_t pos2 = buf.find("INFO", pos);
					if ( pos2 ) {
						buf = buf.substr( pos2 + 5 );
					} else {
						buf = buf.substr( pos + 2, 7 ) + buf.substr( pos + 2 + 8 );
					}
				}
			}
			Debug(Debug::Verbose) + _debug_date() + self_thread() << buf.c_str();
			buf.clear();
		} else {
			buf += (char)c;
		}
		return 0;
	}
	std::string buf;
};


static void mergeControlLists( libcamera::ControlList& dest, libcamera::ControlList& src )
{
	for ( auto it : src ) {
		dest.set( it.first, it.second );
	}
}


static uint32_t stride64( uint32_t value )
{
	return ((value >> 6) << 6) + (((value >> 6) & 1) << 6);
}


LinuxCamera::LinuxCamera()
	: mLivePreview( false )
	, mWidth( 1280 )
	, mHeight( 720 )
	, mFps( 60 )
	, mShutterSpeed( 0 )
	, mISO( 0 )
	, mSharpness( 0.25f )
	, mBrightness( 0.0f )
	, mContrast( 1.0f )
	, mSaturation( 1.0f )
	, mVflip( false )
	, mHflip( false )
	, mWhiteBalance( "auto" )
	, mNightISO( 0 )
	, mNightBrightness( 0.0f )
	, mNightContrast( 1.0f )
	, mNightSaturation( 1.0f )
	, mCurrentFramerate( 0 )
	, mExposureTime( 0 )
	, mNightMode( false )
	, mRawStreamConfiguration( nullptr )
	, mPreviewStreamConfiguration( nullptr )
	, mVideoStreamConfiguration( nullptr )
	, mStillStreamConfiguration( nullptr )
	, mPreviewSurfaceSet( false )
{
	fDebug();

	LibCameraStream* test = new LibCameraStream();

	libcamera::logSetStream( test, true );

	if ( sCameraManager == nullptr ) {
		sCameraManager = std::make_unique<libcamera::CameraManager>();
		sCameraManager->start();

		if ( sCameraManager->cameras().empty() ) {
			gError() << "No camera found on this system";
			sCameraManager->stop();
			return;
		}
	}

	gDebug() << "libcamera available cameras :";
	for ( auto const& camera : sCameraManager->cameras() ) {
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
						name = *model;
					}
					break;
			}
		}
		gDebug() << "    " << name << " (" << camera->id() << ")";
	}
}


LinuxCamera::~LinuxCamera()
{
	fDebug();
}


void LinuxCamera::Start()
{
	fDebug();

	if ( dynamic_cast<LiveOutput*>(mLiveEncoder) != nullptr ) {
		mLivePreview = true;
		mPreviewSurface = new DRMSurface();
	}

	mCamera = sCameraManager->get( sCameraManager->cameras()[0]->id() );
	mCamera->acquire();

	libcamera::StreamRoles roles;
//	roles.push_back( libcamera::StreamRole::Raw );
	roles.push_back( libcamera::StreamRole::Viewfinder );
	roles.push_back( libcamera::StreamRole::VideoRecording );
//	roles.push_back( libcamera::StreamRole::StillCapture );
	mCameraConfiguration = mCamera->generateConfiguration( roles );

	uint32_t iStream = 0;
	if ( std::find( roles.begin(), roles.end(), libcamera::StreamRole::Raw ) != roles.end() ) {
		mRawStreamConfiguration = &mCameraConfiguration->at(iStream++);
	}
	if ( std::find( roles.begin(), roles.end(), libcamera::StreamRole::Viewfinder ) != roles.end() ) {
		mPreviewStreamConfiguration = &mCameraConfiguration->at(iStream++);
	}
	if ( std::find( roles.begin(), roles.end(), libcamera::StreamRole::VideoRecording ) != roles.end() ) {
		mVideoStreamConfiguration = &mCameraConfiguration->at(iStream++);
	}
	if ( std::find( roles.begin(), roles.end(), libcamera::StreamRole::StillCapture ) != roles.end() ) {
		mStillStreamConfiguration = &mCameraConfiguration->at(iStream++);
	}

	if ( mRawStreamConfiguration ) {
		mRawStreamConfiguration->pixelFormat = libcamera::formats::SBGGR10_CSI2P;
		mRawStreamConfiguration->size.width = mWidth;
		mRawStreamConfiguration->size.height = mHeight;
		mRawStreamConfiguration->bufferCount = 2;
	}
	if ( mPreviewStreamConfiguration ) {
		mPreviewStreamConfiguration->pixelFormat = libcamera::formats::YUV420;
		mPreviewStreamConfiguration->size.width = ( mLivePreview ? mPreviewSurface->mode()->hdisplay : mWidth );
		mPreviewStreamConfiguration->size.height = ( mLivePreview ? mPreviewSurface->mode()->vdisplay : mHeight );
		mPreviewStreamConfiguration->bufferCount = 1;
	}
	if ( mVideoStreamConfiguration ) {
		mVideoStreamConfiguration->pixelFormat = libcamera::formats::YUV420;
		mVideoStreamConfiguration->size.width = mWidth;
		mVideoStreamConfiguration->size.height = mHeight;
		mVideoStreamConfiguration->bufferCount = 1;
	}
	if ( mStillStreamConfiguration ) {
		// TODO
	}

	mCameraConfiguration->transform = ( mHflip ? libcamera::Transform::HFlip : libcamera::Transform::Identity ) | ( mVflip ? libcamera::Transform::VFlip : libcamera::Transform::Identity );

	mCameraConfiguration->validate();
	if ( mRawStreamConfiguration ) {
		gDebug() << "Validated raw configuration is: " << mRawStreamConfiguration->toString();
	}
	if ( mPreviewStreamConfiguration ) {
		gDebug() << "Validated viewfinder configuration is: " << mPreviewStreamConfiguration->toString();
	}
	if ( mVideoStreamConfiguration ) {
		gDebug() << "Validated video configuration is: " << mVideoStreamConfiguration->toString();
	}
	if ( mStillStreamConfiguration ) {
		gDebug() << "Validated still configuration is: " << mStillStreamConfiguration->toString();
	}

	mCamera->configure( mCameraConfiguration.get() );
	mWidth = mVideoStreamConfiguration->size.width;
	mHeight = mVideoStreamConfiguration->size.height;

	libcamera::ControlList controls;
	controls.set( libcamera::controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ 1000000 / mFps, 1000000 / mFps }) );
	controls.set( libcamera::controls::ExposureTime, mShutterSpeed );
	controls.set( libcamera::controls::AnalogueGain, (float)mISO / 100.0f );
	controls.set( libcamera::controls::Sharpness, mSharpness );
	controls.set( libcamera::controls::Brightness, mBrightness );
	controls.set( libcamera::controls::Contrast, mContrast );
	controls.set( libcamera::controls::Saturation, mSaturation );
	if ( mWhiteBalance == "incandescent" ) {
		controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbIncandescent );
	} else if ( mWhiteBalance == "tungsten" ) {
		controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbTungsten );
	} else if ( mWhiteBalance == "fluorescent" ) {
		controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbFluorescent );
	} else if ( mWhiteBalance == "indoor" ) {
		controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbIndoor );
	} else if ( mWhiteBalance == "daylight" ) {
		controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbDaylight );
	} else if ( mWhiteBalance == "cloudy" ) {
		controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbCloudy );
	} else if ( mWhiteBalance == "custom" ) {
		controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbCustom );
	}
	if ( mExposureMode == "short" ) {
		controls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureShort );
	} else if ( mExposureMode == "long" ) {
		controls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureLong );
	} else if ( mExposureMode == "custom" ) {
		controls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureCustom );
	}

	if ( mRawStreamConfiguration ) {
		mControlListQueue[mRawStreamConfiguration->stream()] = libcamera::ControlList();
	}
	if ( mPreviewStreamConfiguration ) {
		mControlListQueue[mPreviewStreamConfiguration->stream()] = libcamera::ControlList();
	}
	if ( mVideoStreamConfiguration ) {
		mControlListQueue[mVideoStreamConfiguration->stream()] = libcamera::ControlList();
	}
	if ( mStillStreamConfiguration ) {
		mControlListQueue[mStillStreamConfiguration->stream()] = libcamera::ControlList();
	}


	libcamera::FrameBufferAllocator* allocator = new libcamera::FrameBufferAllocator( mCamera );

	for ( libcamera::StreamConfiguration& cfg : *mCameraConfiguration ) {
		if ( allocator->allocate(cfg.stream()) < 0 ) {
			gError() << "Cannot allocate buffers";
			return;
		}
		const std::vector<std::unique_ptr<libcamera::FrameBuffer>>& buffers = allocator->buffers(cfg.stream());
		gDebug() << "Allocated " << buffers.size() << " buffers for stream " << cfg.toString();

		for ( uint32_t i = 0; i < buffers.size(); i++ ) {
			std::unique_ptr<libcamera::Request> request = mCamera->createRequest();
			if ( !request ) {
				gError() << "Cannot create request for stream " << cfg.toString();
				return;
			}
			const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
			if ( request->addBuffer( cfg.stream(), buffer.get() ) < 0 ) {
				gError() << "Cannot set buffer into request for stream " << cfg.toString();
				return;
			}
			/*
			libcamera::ControlList &controls = request->controls();
			controls.set(libcamera::controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ 1000000 / mFps, 1000000 / mFps }));
			*/
			request->controls().merge( controls );
			mRequests.push_back(std::move(request));
			if ( mLivePreview and &cfg == mPreviewStreamConfiguration ) {
				printf("FD : %d\n", buffer->planes()[0].fd.get());
				mPreviewFrameBuffers[buffer.get()] = new DRMFrameBuffer( mPreviewStreamConfiguration->size.width, mPreviewStreamConfiguration->size.height, stride64(mPreviewStreamConfiguration->size.width), DRM_FORMAT_YUV420, 0, buffer->planes()[0].fd.get() );
			}
		}
	}

	mCamera->requestCompleted.connect( this, &LinuxCamera::requestComplete );
	mCamera->start();

	for ( std::unique_ptr<libcamera::Request>& request : mRequests ) {
		mCamera->queueRequest(request.get());
	}
/*
	mLiveThread = new HookThread<LinuxCamera>( "cam_live", this, &LinuxCamera::LiveThreadRun );
	mLiveThread->Start();
	mLiveThread->setPriority( 99, 3 );
*/
}


void LinuxCamera::requestComplete( libcamera::Request* request )
{
	if ( request->status() != libcamera::Request::RequestCancelled ) {
		const libcamera::ControlList& metadata = request->metadata();
		const libcamera::Request::BufferMap& buffers = request->buffers();
/*
		for (const auto& ctrl : metadata) {
			const libcamera::ControlId* id = libcamera::controls::controls.at(ctrl.first);
			const libcamera::ControlValue& value = ctrl.second;
			std::cout << "\t" << id->name() << " = " << value.toString()
				<< std::endl;
		}
*/

		mCurrentFramerate = 1000000L / *metadata.get( libcamera::controls::FrameDuration );
		mExposureTime = *metadata.get( libcamera::controls::ExposureTime );

		if ( buffers.size() > 0 ) {
			libcamera::ControlList mergedControls;
			for ( auto bufferPair : buffers ) {
				const libcamera::Stream* stream = bufferPair.first;
				libcamera::FrameBuffer* buffer = bufferPair.second;
				if ( stream == mPreviewStreamConfiguration->stream() ) {
					if ( mLivePreview and mPreviewSurface and not mPreviewSurfaceSet ) {
						mPreviewSurfaceSet = true;
						mPreviewSurface->Show( mPreviewFrameBuffers[buffer] );
					}
				}
				mControlListQueueMutex.lock();
				libcamera::ControlList& controls = mControlListQueue[stream];
				if ( controls.size() > 0 ) {
					mergeControlLists( mergedControls, controls );
					controls.clear();
				}
				mControlListQueueMutex.unlock();
			}
			request->reuse( libcamera::Request::ReuseBuffers );
			mergeControlLists( request->controls(), mergedControls );
			mCamera->queueRequest( request );
		}
	}
}


void LinuxCamera::Pause()
{
}


void LinuxCamera::Resume()
{
}


bool LinuxCamera::LiveThreadRun()
{
	return true;
}


bool LinuxCamera::RecordThreadRun()
{
	return true;
}


bool LinuxCamera::TakePictureThreadRun()
{
	return true;
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
	libcamera::ControlList controls;

	mNightMode = night_mode;
	if ( night_mode ) {
		controls.set( libcamera::controls::AnalogueGain, (float)mNightISO / 100.0f );
		controls.set( libcamera::controls::Brightness, mNightBrightness );
		controls.set( libcamera::controls::Contrast, mNightContrast );
		controls.set( libcamera::controls::Saturation, mNightSaturation );
//		controls.set( libcamera::controls::draft::NoiseReductionMode, libcamera::controls::draft::NoiseReductionModeEnum::NoiseReductionModeHighQuality );
	} else {
		controls.set( libcamera::controls::AnalogueGain, (float)mISO / 100.0f );
		controls.set( libcamera::controls::Brightness, mBrightness );
		controls.set( libcamera::controls::Contrast, mContrast );
		controls.set( libcamera::controls::Saturation, mSaturation );
//		controls.set( libcamera::controls::draft::NoiseReductionMode, libcamera::controls::draft::NoiseReductionModeEnum::NoiseReductionModeOff );
	}

	mControlListQueueMutex.lock();

	if ( mRawStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mRawStreamConfiguration->stream()], controls );
	}
	if ( mPreviewStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mPreviewStreamConfiguration->stream()], controls );
	}
	if ( mVideoStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mVideoStreamConfiguration->stream()], controls );
	}
	if ( mStillStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mStillStreamConfiguration->stream()], controls );
	}

	mControlListQueueMutex.unlock();
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

const uint32_t LinuxCamera::width()
{
	return mWidth;
}


const uint32_t LinuxCamera::height()
{
	return mHeight;
}


const uint32_t LinuxCamera::framerate()
{
	return mCurrentFramerate;
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
	return mNightMode;
}


const uint32_t LinuxCamera::shutterSpeed()
{
	return mExposureTime;
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
