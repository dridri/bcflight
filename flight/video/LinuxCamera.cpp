#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include "LinuxCamera.h"
#include "LiveOutput.h"
#include "Debug.h"
#include <thread>
#include <sys/mman.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

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


static int xioctl( int fd, unsigned long ctl, void* arg )
{
	int ret, num_tries = 10;

	do {
		ret = ioctl( fd, ctl, arg );
	} while (ret == -1 && errno == EINTR && num_tries-- > 0 );

	return ret;
}


static void mergeControlLists( libcamera::ControlList& dest, const libcamera::ControlList& src )
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
	, mHDR( false )
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
	, mAwbGains({ 0.0f, 0.0f })
	, mNightMode( false )
	, mCamera( nullptr )
	, mRawStreamConfiguration( nullptr )
	, mPreviewStreamConfiguration( nullptr )
	, mVideoStreamConfiguration( nullptr )
	, mStillStreamConfiguration( nullptr )
	, mPreviewSurface( nullptr )
	, mPreviewSurfaceSet( false )
	, mStopping( false )
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


LuaValue LinuxCamera::infos()
{
	LuaValue ret;

	if ( not mCamera ) {
		ret["Device"] = "None";
		return ret;
	}

	ret["Device"] = mCamera->id();
	if ( mPreviewStreamConfiguration ) {
		ret["Viewfinder Configuration"] = mPreviewStreamConfiguration->toString();
	}
	if ( mVideoStreamConfiguration ) {
		ret["Video Configuration"] = mVideoStreamConfiguration->toString();
	}
	if ( mStillStreamConfiguration ) {
		ret["Still Configuration"] = mStillStreamConfiguration->toString();
	}

	ret["Resolution"] = std::to_string(mWidth) + "x" + std::to_string(mHeight);
	ret["Framerate"] = mFps;
	ret["HDR"] = ( mHDR ? "on" : "off" );

	if ( mCamera->controls().find( libcamera::controls::MAX_LATENCY) != mCamera->controls().end() ) {
		ret["Max Latency"] = mCamera->controls().at( libcamera::controls::MAX_LATENCY ).toString();
	}

	return ret;
}


void LinuxCamera::Start()
{
	fDebug();
	Camera::Start();

	if ( sCameraManager->cameras().size() == 0 ) {
		gError() << "No camera detected !";
		return;
	}

	if ( dynamic_cast<LiveOutput*>(mLiveEncoder) != nullptr and !mPreviewSurface ) {
		mLivePreview = true;
		mPreviewSurface = new DRMSurface();
	}

	mCamera = sCameraManager->get( sCameraManager->cameras()[0]->id() );
	mCamera->acquire();

	std::vector<libcamera::StreamRole> roles;
	// roles.push_back( libcamera::StreamRole::Raw );
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

	uint32_t bufferCount = 1;

	if ( mRawStreamConfiguration ) {
		mRawStreamConfiguration->pixelFormat = libcamera::formats::SRGGB12_CSI2P;
		mRawStreamConfiguration->size.width = mWidth;
		mRawStreamConfiguration->size.height = mHeight;
		mRawStreamConfiguration->bufferCount = 1;
	}
	if ( mPreviewStreamConfiguration ) {
		mPreviewStreamConfiguration->pixelFormat = libcamera::formats::YUV420;
		mPreviewStreamConfiguration->size.width = ( mLivePreview ? mPreviewSurface->mode()->hdisplay : mWidth );
		mPreviewStreamConfiguration->size.height = ( mLivePreview ? mPreviewSurface->mode()->vdisplay : mHeight );
		mPreviewStreamConfiguration->bufferCount = bufferCount;
	}
	if ( mVideoStreamConfiguration ) {
		mVideoStreamConfiguration->pixelFormat = libcamera::formats::YUV420;
		mVideoStreamConfiguration->size.width = mWidth;
		mVideoStreamConfiguration->size.height = mHeight;
		mVideoStreamConfiguration->bufferCount = bufferCount;
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

	if ( mHDR ) {
		setHDR( mHDR, true );
	}

	mCamera->configure( mCameraConfiguration.get() );
	if ( mVideoStreamConfiguration ) {
		mWidth = mVideoStreamConfiguration->size.width;
		mHeight = mVideoStreamConfiguration->size.height;
	}

	mAllControls.set( libcamera::controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ 1000000 / mFps, 1000000 / mFps }) );
	mAllControls.set( libcamera::controls::ExposureTime, mShutterSpeed );
	mAllControls.set( libcamera::controls::AnalogueGain, (float)mISO / 100.0f );
	mAllControls.set( libcamera::controls::Sharpness, mSharpness );
	mAllControls.set( libcamera::controls::Brightness, mBrightness );
	mAllControls.set( libcamera::controls::Contrast, mContrast );
	mAllControls.set( libcamera::controls::Saturation, mSaturation );
	mAllControls.set( libcamera::controls::AfMode, libcamera::controls::AfModeAuto ); // AfModeContinuous
	mAllControls.set( libcamera::controls::draft::SceneFlicker, libcamera::controls::draft::SceneFickerOff );
	// mAllControls.set( libcamera::controls::AeMeteringMode, libcamera::controls::MeteringMatrix );
	// mAllControls.set( libcamera::controls::AeMeteringMode, libcamera::controls::MeteringSpot );
	mAllControls.set( libcamera::controls::AeMeteringMode, libcamera::controls::MeteringCentreWeighted );
	mAllControls.set( libcamera::controls::draft::NoiseReductionMode, libcamera::controls::draft::NoiseReductionModeEnum::NoiseReductionModeFast );
	setWhiteBalance( mWhiteBalance, &mAllControls );
	if ( mExposureMode == "short" ) {
		mAllControls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureShort );
	} else if ( mExposureMode == "long" ) {
		mAllControls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureLong );
	} else if ( mExposureMode == "custom" ) {
		mAllControls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureCustom );
	} else {
		mAllControls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureNormal );
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


	mAllocator = new libcamera::FrameBufferAllocator( mCamera );

	for ( libcamera::StreamConfiguration& cfg : *mCameraConfiguration ) {
		if ( mAllocator->allocate(cfg.stream()) < 0 ) {
			gError() << "Cannot allocate buffers";
			return;
		}
		const std::vector<std::unique_ptr<libcamera::FrameBuffer>>& buffers = mAllocator->buffers(cfg.stream());
		gDebug() << "Allocated " << buffers.size() << " buffers for stream " << cfg.toString();
		for ( uint32_t i = 0; i < buffers.size(); i++ ) {
			const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
			size_t sz = 0;
			for ( auto plane : buffer->planes() ) {
				sz += plane.length;
			}
			uint8_t* ptr = static_cast<uint8_t*>( mmap( nullptr, sz, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->planes()[0].fd.get(), 0 ) );
			if ( !ptr ) {
				gError() << "Failed to mmap buffer for stream " << cfg.toString();
			}
			gDebug() << "mmap buffer for stream " << cfg.toString() << " : " << ptr;
			mPlaneBuffers[buffer.get()] = ptr;
		}
		if ( mLivePreview and &cfg == mPreviewStreamConfiguration ) {
			for ( uint32_t i = 0; i < buffers.size(); i++ ) {
				const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
				mPreviewFrameBuffers[buffer.get()] = new DRMFrameBuffer( mPreviewStreamConfiguration->size.width, mPreviewStreamConfiguration->size.height, stride64(mPreviewStreamConfiguration->size.width), DRM_FORMAT_YUV420, 0, buffer->planes()[0].fd.get() );
			}
		}
	}

	for ( uint32_t i = 0; i < bufferCount; i++ ) {
		std::unique_ptr<libcamera::Request> request = mCamera->createRequest();
		for ( libcamera::StreamConfiguration& cfg : *mCameraConfiguration ) {
			const std::vector<std::unique_ptr<libcamera::FrameBuffer>>& buffers = mAllocator->buffers(cfg.stream());
			const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
			if ( request->addBuffer( cfg.stream(), buffer.get() ) < 0 ) {
				gError() << "Cannot set buffer into request for stream " << cfg.toString();
				return;
			}
		}
		mRequests.push_back(std::move(request));
	}

	if ( mVideoEncoder ) {
		mVideoEncoder->SetInputResolution( mWidth, mHeight );
	}
	if ( mLiveEncoder ) {
		mLiveEncoder->SetInputResolution( mWidth, mHeight );
	}

	mCamera->requestCompleted.connect( this, &LinuxCamera::requestComplete );
	mCamera->start(&mAllControls);

	for ( std::unique_ptr<libcamera::Request>& request : mRequests ) {
		mRequestsAliveMutex.lock();
		mRequestsAlive.insert( request.get() );
		mRequestsAliveMutex.unlock();
		mCamera->queueRequest( request.get() );
	}
}

void LinuxCamera::Stop()
{
/*
	{
		// We don't want QueueRequest to run asynchronously while we stop the camera.
		std::lock_guard<std::mutex> lock(camera_stop_mutex_);
		if ( mCamera->stop() ) {
			gError() << "Failed to stop camera";
		}
	}

	mCamera->requestCompleted.disconnect( this, &LinuxCamera::requestComplete );
	mRequests.clear();

	mControlListQueueMutex.lock();
	mControlListQueue.clear();
	mControlListQueueMutex.unlock();

	mCamera->release();
	mCamera.reset();

	gDebug() << "Camera stopped!";
*/
}


void LinuxCamera::requestComplete( libcamera::Request* request )
{
	if ( request->status() == libcamera::Request::RequestCancelled ) {
		if ( mStopping ) {
			mRequestsAliveMutex.lock();
			mRequestsAlive.erase( request );
			mRequestsAliveMutex.unlock();
		}
	} else {
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
		mAwbGains = *metadata.get( libcamera::controls::ColourGains );
		mColorTemperature = *metadata.get( libcamera::controls::ColourTemperature );
		/*
		gDebug() << "Framerate: " << mCurrentFramerate;
		gDebug() << "Exposure time: " << mExposureTime;
		gDebug() << "AWB gains: " << mAwbGains[0] << ", " << mAwbGains[1];
		gDebug() << "Color temperature: " << mColorTemperature;
		*/
		if ( buffers.size() > 0 ) {
			libcamera::ControlList mergedControls;
			for ( auto bufferPair : buffers ) {
				const libcamera::Stream* stream = bufferPair.first;
				libcamera::FrameBuffer* buffer = bufferPair.second;
				if ( stream == mPreviewStreamConfiguration->stream() ) {
					if ( mLivePreview and mPreviewSurface and ( mPreviewFrameBuffers.size() > 1 or not mPreviewSurfaceSet ) ) {
						mPreviewSurfaceSet = true;
						mPreviewSurface->Show( mPreviewFrameBuffers[buffer] );
					}
				}
				if ( mPreviewStreamConfiguration and stream == mPreviewStreamConfiguration->stream() ) {
					if ( mLiveEncoder ) {
						auto ts = metadata.get(libcamera::controls::SensorTimestamp);
						int64_t timestamp_ns = ts ? *ts : buffer->metadata().timestamp;
						size_t sz = 0;
						for ( auto plane : buffer->planes() ) {
							sz += plane.length;
						}
						mLiveEncoder->EnqueueBuffer( sz, mPlaneBuffers[buffer], timestamp_ns / 1000, buffer->planes()[0].fd.get() );
					}
				}
				if ( mVideoStreamConfiguration and stream == mVideoStreamConfiguration->stream() ) {
					if ( mVideoEncoder ) {
						auto ts = metadata.get(libcamera::controls::SensorTimestamp);
						int64_t timestamp_ns = ts ? *ts : buffer->metadata().timestamp;
						size_t sz = 0;
						for ( auto plane : buffer->planes() ) {
							sz += plane.length;
						}
						mVideoEncoder->EnqueueBuffer( sz, mPlaneBuffers[buffer], timestamp_ns / 1000, buffer->planes()[0].fd.get() );
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


void LinuxCamera::setHDR( bool hdr, bool force )
{
	if ( mHDR == hdr and not force ) {
		return;
	}

	mRequestsAliveMutex.lock();
	bool wasRunning = mRequestsAlive.size() > 0;
	mRequestsAliveMutex.unlock();
	if ( wasRunning ) {
		mStopping = true;
		mCamera->stop();
		mRequestsAliveMutex.lock();
		while ( mRequestsAlive.size() > 0 ) {
			mRequestsAliveMutex.unlock();
			usleep( 100 );
			mRequestsAliveMutex.lock();
		}
		mRequestsAliveMutex.unlock();
		mStopping = false;
	}

	{
		bool ok = false;
		int fd = -1;
		v4l2_control ctrl { V4L2_CID_WIDE_DYNAMIC_RANGE, hdr };
		for ( int i = 0; i < 4 && !ok; i++ ) {
			std::string dev = std::string("/dev/v4l-subdev") + (char)('0' + i);
			if ( (fd = open(dev.c_str(), O_RDWR, 0)) >= 0 ) {
				ok = !xioctl( fd, VIDIOC_S_CTRL, &ctrl );
				close(fd);
			}
		}
		if ( !ok ) {
			gWarning() << "WARNING: Cannot " << ( hdr ? "en" : "dis" ) << "able HDR mode";
		} else {
			mHDR = hdr;
		}
	}

	if ( wasRunning ) {
		mCamera->configure( mCameraConfiguration.get() );
		mCamera->start( &mAllControls );
		for ( std::unique_ptr<libcamera::Request>& request : mRequests ) {
			mRequestsAliveMutex.lock();
			mRequestsAlive.insert( request.get() );
			mRequestsAliveMutex.unlock();
			request->reuse( libcamera::Request::ReuseBuffers );
			mCamera->queueRequest(request.get());
		}
	}
}


void LinuxCamera::getLensShader( Camera::LensShaderColor* r, Camera::LensShaderColor* g, Camera::LensShaderColor* b )
{
}


void LinuxCamera::setLensShader( const Camera::LensShaderColor& r, const Camera::LensShaderColor& g, const Camera::LensShaderColor& b )
{
}


std::string LinuxCamera::switchExposureMode()
{
	return "";
}


bool LinuxCamera::setWhiteBalance( const std::string& mode, libcamera::ControlList* controlsList )
{
	static const std::map< std::string, libcamera::controls::AwbModeEnum > awbModes = {
		{ "auto", libcamera::controls::AwbModeEnum::AwbAuto },
		{ "incandescent", libcamera::controls::AwbModeEnum::AwbIncandescent },
		{ "tungsten", libcamera::controls::AwbModeEnum::AwbTungsten },
		{ "fluorescent", libcamera::controls::AwbModeEnum::AwbFluorescent },
		{ "indoor", libcamera::controls::AwbModeEnum::AwbIndoor },
		{ "daylight", libcamera::controls::AwbModeEnum::AwbDaylight },
		{ "cloudy", libcamera::controls::AwbModeEnum::AwbCloudy },
		{ "custom", libcamera::controls::AwbModeEnum::AwbCustom },
	};


	if ( awbModes.find( mode ) != awbModes.end() ) {
		if ( controlsList ) {
			(*controlsList).set( libcamera::controls::AwbMode, awbModes.at( mode ) );
		} else {
			libcamera::ControlList controls;
			(controlsList ? *controlsList : controls).set( libcamera::controls::AwbMode, awbModes.at( mode ) );
			pushControlList( controls );
		}
		mWhiteBalance = mode;
		return true;
	}

	return false;
}


std::string LinuxCamera::switchWhiteBalance()
{
	static const std::map< std::string, std::string > nextMode = {
		{ "auto", "incandescent" },
		{ "incandescent", "fluorescent" },
		{ "fluorescent", "daylight" },
		{ "daylight", "cloudy" },
		{ "cloudy", "auto" },
		{ "custom", "auto" },
	};
	if ( nextMode.find( mWhiteBalance ) == nextMode.end() ) {
		setWhiteBalance( "auto" );
		libcamera::ControlList controls;
		controls.set( libcamera::controls::ColourGains, libcamera::Span<const float, 2>({ 0.0f, 0.0f }) );
		pushControlList( controls );
		return "auto";
	}
	std::string newMode = nextMode.at( mWhiteBalance );
	setWhiteBalance( newMode );
	return newMode;
}


std::string LinuxCamera::lockWhiteBalance()
{
	libcamera::ControlList controls;

	if ( mAwbGains[0] < 0.0f or mAwbGains[1] < 0.0f or mAwbGains[0] > 16.0f or mAwbGains[1] > 16.0f ) {
		return mWhiteBalance;
	}

	auto span = libcamera::Span<const float, 2>({ mAwbGains[0], mAwbGains[1] });

	controls.set( libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbCustom );
	controls.set( libcamera::controls::ColourGains, span );

	pushControlList( controls );
	char buf[64];
	snprintf( buf, sizeof(buf), "R%.2fB%.2f", span[0], span[1] );
	mWhiteBalance = buf;
	return mWhiteBalance;
}


void LinuxCamera::updateSettings()
{
	libcamera::ControlList controls;

	controls.set( libcamera::controls::AnalogueGain, (float)mISO / 100.0f );
	controls.set( libcamera::controls::Brightness, mBrightness );
	controls.set( libcamera::controls::Contrast, mContrast );
	controls.set( libcamera::controls::Saturation, mSaturation );
	setWhiteBalance( mWhiteBalance, &controls );
	if ( mExposureMode == "short" ) {
		controls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureShort );
	} else if ( mExposureMode == "long" ) {
		controls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureLong );
	} else if ( mExposureMode == "custom" ) {
		controls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureCustom );
	} else {
		controls.set( libcamera::controls::AeExposureMode, libcamera::controls::AeExposureModeEnum::ExposureNormal );
	}
//	controls.set( libcamera::controls::draft::NoiseReductionMode, libcamera::controls::draft::NoiseReductionModeEnum::NoiseReductionModeOff );

	pushControlList( controls );
}


void LinuxCamera::pushControlList( const libcamera::ControlList& list )
{
	mControlListQueueMutex.lock();

	if ( mRawStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mRawStreamConfiguration->stream()], list );
	}
	if ( mPreviewStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mPreviewStreamConfiguration->stream()], list );
	}
	if ( mVideoStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mVideoStreamConfiguration->stream()], list );
	}
	if ( mStillStreamConfiguration ) {
		mergeControlLists( mControlListQueue[mStillStreamConfiguration->stream()], list );
	}

	mControlListQueueMutex.unlock();
}


void LinuxCamera::setShutterSpeed( uint32_t value )
{
}


void LinuxCamera::setISO( int32_t value )
{
	fDebug( value );
	mISO = value;
	libcamera::ControlList controls;
	mAllControls.set( libcamera::controls::AnalogueGain, (float)mISO / 100.0f );
	pushControlList( controls );
}


void LinuxCamera::setSaturation( float value )
{
	fDebug( value );
	mSaturation = value;
	libcamera::ControlList controls;
	controls.set( libcamera::controls::Saturation, mSaturation );
	pushControlList( controls );
}


void LinuxCamera::setContrast( float value )
{
	fDebug( value );
	mContrast = value;
	libcamera::ControlList controls;
	controls.set( libcamera::controls::Contrast, mContrast );
	pushControlList( controls );
}


void LinuxCamera::setBrightness( float value )
{
	fDebug( value );
	mBrightness = value;
	libcamera::ControlList controls;
	controls.set( libcamera::controls::Brightness, mBrightness );
	pushControlList( controls );
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
	return false;
}


const std::string LinuxCamera::exposureMode()
{
	return "";
}


const std::string LinuxCamera::whiteBalance()
{
	return mWhiteBalance;
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
	return mISO;
}


const float LinuxCamera::saturation()
{
	return mSaturation;
}


const float LinuxCamera::contrast()
{
	return mContrast;
}


const float LinuxCamera::brightness()
{
	return mBrightness;
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
	return 0;
}


const std::string LinuxCamera::recordFilename()
{
	return "";
}


uint32_t* LinuxCamera::getFileSnapshot( const std::string& filename, uint32_t* width, uint32_t* height, uint32_t* bpp )
{
	return nullptr;
}
