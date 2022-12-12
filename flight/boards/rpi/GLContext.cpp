#include <iostream>
#include <algorithm>
#include "Debug.h"
#include "GLContext.h"
#include <vc_dispmanx_types.h>
#include <bcm_host.h>
#include <fcntl.h>

#define OPTIMUM_WIDTH 840
#define OPTIMUM_HEIGHT 480

GLContext* GLContext::sInstance = nullptr;


GLContext* GLContext::instance()
{
	if ( !sInstance ) {
		sInstance = new GLContext();
	}
	return sInstance;
}


GLContext::GLContext()
	: Thread( "OpenGL" )
	, mReady( false )
	, mWidth( 0 )
	, mHeight( 0 )
{
	Start();
}


GLContext::~GLContext()
{
}


const EGLDisplay GLContext::eglDisplay() const
{
	return mEGLDisplay;
}


const EGLContext GLContext::eglContext() const
{
	return mEGLContext;
}


bool GLContext::run()
{
	if ( !mReady ) {
		mReady = true;
		Initialize( 1280, 720 );
	}

	mContextQueueMutex.lock();
	while ( mContextQueue.size() > 0 ) {
		std::function<void()> f = mContextQueue.front();
		mContextQueue.pop_front();
		mContextQueueMutex.unlock();
		f();
		mContextQueueMutex.lock();
	}
	mContextQueueMutex.unlock();


	glClear( GL_COLOR_BUFFER_BIT );

	mLayersMutex.lock();
	for ( const Layer& layer : mLayers ) {
		if ( layer.fct ) {
			layer.fct();
		} else if ( layer.glId >= 0 ) {
			// TODO
		}
	}
	mLayersMutex.unlock();

	SwapBuffers();
	return true;
}


void GLContext::runOnGLThread( const std::function<void()>& f, bool wait )
{
	if ( wait ) {
		bool finished = false;
		mContextQueueMutex.lock();
		mContextQueue.emplace_back([&finished, f]() {
			f();
			finished = true;
		});
		mContextQueueMutex.unlock();
		while ( not finished ) {
			usleep( 10 * 1000 );
		}
	} else {
		mContextQueueMutex.lock();
		mContextQueue.emplace_back( f );
		mContextQueueMutex.unlock();
	}
}


uint32_t GLContext::addLayer( int32_t x, int32_t y, int32_t width, int32_t height, GLenum format, int32_t layerIndex )
{
	uint32_t tex = -1;

	runOnGLThread([&tex,format,width,height]() {
		printf("A\n");
		glGenTextures( 1, &tex );
		printf("B, %d\n", tex);

		glBindTexture( GL_TEXTURE_2D, tex );
		glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}, true);

	Layer layer = {
		.index = layerIndex,
		.x = x,
		.y = y,
		.width = (uint32_t)width,
		.height = (uint32_t)height,
		.glId = tex
	};
	mLayersMutex.lock();
	mLayers.emplace_back( layer );
	std::sort( mLayers.begin(), mLayers.end(), [](Layer& a, Layer& b) {
		return a.index < b.index;
	});
	mLayersMutex.unlock();

	return tex;
}


uint32_t GLContext::addLayer( const std::function<void ()>& f, int32_t layerIndex )
{
	Layer layer = {
		.index = layerIndex,
		.fct = f
	};
	mLayersMutex.lock();
	mLayers.emplace_back( layer );
	std::sort( mLayers.begin(), mLayers.end(), [](Layer& a, Layer& b) {
		return a.index < b.index;
	});
	mLayersMutex.unlock();
}


EGLImageKHR GLContext::eglImage( uint32_t glImage )
{
	EGLImageKHR eglVideoImage = 0;

	runOnGLThread([&eglVideoImage,glImage]() {
		PFNEGLCREATEIMAGEKHRPROC eglCreateImage = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
		gDebug() << "eglCreateImage (" << glImage << ") : " << (uintptr_t)eglCreateImage;
		gDebug() << "eglGetCurrentDisplay : " << eglGetCurrentDisplay();
		gDebug() << "eglGetCurrentContext : " << eglGetCurrentContext();
		eglVideoImage = eglCreateImage( eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D_KHR, reinterpret_cast<EGLClientBuffer>(glImage), nullptr );
		gDebug() << "EGL error : " << eglGetError();
	}, true);

	return eglVideoImage;
}



#ifdef VARIANT_4
using namespace std;
extern std::mutex __global_rpi_drm_mutex;
extern int __global_rpi_drm_fd;
EGLConfig GLContext::getDisplay()
{
	drmModeRes* resources = nullptr;
/*
	for ( int dev = 0; dev <= 1; dev++ ) {
		mDevice = open( ( string("/dev/dri/card") + to_string(dev) ).c_str(), O_RDWR | O_CLOEXEC );
		if ( mDevice < 0 ) {
			gDebug() << "FATAL : Cannot open EGL device \"" << ( string("/dev/dri/card") + to_string(dev) ) << "\" : " << strerror(errno);
			exit(0);
		} else {
			resources = drmModeGetResources( mDevice );
			if ( resources ) {
				break;
			}
			close( mDevice );
		}
	}
*/
	mDevice = open("/proc/6872/fd/3", O_RDWR);
	printf("mDevice : %d\n", mDevice);
	resources = drmModeGetResources( mDevice );
	drmSetMaster(mDevice);
	printf("drmSetMaster() %s\n", strerror(errno));

	if (__global_rpi_drm_fd < 0) {
		drmSetMaster( mDevice );
		__global_rpi_drm_fd = mDevice;
	} else {
		close( mDevice );
		mDevice = __global_rpi_drm_fd;
	}

	if ( resources == nullptr ) {
		fprintf(stderr, "Unable to get DRM resources\n");
		exit(0);
	}

	printf("=============================== %u =============================\n", mDevice);

	drmModeConnector* connector = nullptr;
	for ( int i = 0; i < resources->count_connectors; i++ ) {
		drmModeConnector* conn = drmModeGetConnector( mDevice, resources->connectors[i] );
		for ( int j = 0; j < conn->count_modes; j++ ) {
			printf( "Connector %d mode %d : '%s'\n", i, j, conn->modes[j].name );
		}
		drmModeFreeConnector( conn );
	}
	for ( int i = 0; i < resources->count_connectors; i++ ) {
		drmModeConnector* conn = drmModeGetConnector( mDevice, resources->connectors[i] );
		if ( /*i == 1 and*/ conn->connection == DRM_MODE_CONNECTED ) {
			connector = conn;
			break;
		} else if ( conn ) {
			drmModeFreeConnector( conn );
		}
	}
	if ( connector == nullptr ) {
		fprintf(stderr, "Unable to get connector\n");
		drmModeFreeResources(resources);
		exit(0);
	}

	mConnectorId = connector->connector_id;
	mMode = connector->modes[0];
	printf("resolution: %ix%i\n", mMode.hdisplay, mMode.vdisplay);

    drmModeEncoder* encoder = ( connector->encoder_id ? drmModeGetEncoder( mDevice, connector->encoder_id ) : nullptr );
	if ( encoder == nullptr ) {
		fprintf(stderr, "Unable to get encoder\n");
		drmModeFreeConnector(connector);
		drmModeFreeResources(resources);
		exit(0);
	}

	mCrtc = drmModeGetCrtc( mDevice, encoder->crtc_id );

	mPlaneId = 0;
	drmModePlaneResPtr planes = drmModeGetPlaneResources( mDevice );
	for ( int i = 0; i < planes->count_planes && mPlaneId == 0; i++ ) {
		drmModePlanePtr plane = drmModeGetPlane( mDevice, planes->planes[i] );
		printf( "Plane %d : %d %d %d %d\n", i, plane->x, plane->y, plane->crtc_x, plane->crtc_y );
		drmModeObjectProperties* props = drmModeObjectGetProperties( mDevice, plane->plane_id, DRM_MODE_OBJECT_ANY );
		for ( int j = 0; j < props->count_props && mPlaneId == 0; j++ ) {
			drmModePropertyRes* prop = drmModeGetProperty( mDevice, props->props[j] );
			if ( strcmp(prop->name, "type") == 0 && props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY ) {
				mPlaneId = plane->plane_id;
				break;
			}
			drmModeFreeProperty(prop);
		}
		drmModeFreeObjectProperties(props);
		drmModeFreePlane(plane);
	}
	printf( "plane_id : 0x%08X\n", mPlaneId );

	drmModeFreeEncoder(encoder);
	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);
	mGbmDevice = gbm_create_device( mDevice );
	mGbmSurface = gbm_surface_create( mGbmDevice, mMode.hdisplay, mMode.vdisplay, GBM_FORMAT_ARGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING );
	return eglGetDisplay( mGbmDevice );
}
#endif


int32_t GLContext::Initialize( uint32_t width, uint32_t height )
{
#ifndef VARIANT_4
	mLayerDisplay = CreateNativeWindow( 2 );
#endif

	EGLint attribList[] =
	{
// 		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
// 		EGL_TRANSPARENT_TYPE,  EGL_TRANSPARENT_RGB,
		EGL_RED_SIZE,          8,
		EGL_GREEN_SIZE,        8,
		EGL_BLUE_SIZE,         8,
		EGL_ALPHA_SIZE,        8,
		EGL_DEPTH_SIZE,        16,
		EGL_STENCIL_SIZE,      0,
// 		EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	EGLint count;
	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

#ifdef VARIANT_4
	mEGLDisplay = getDisplay();
#else
	mEGLDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
#endif


	eglInitialize( mEGLDisplay, &majorVersion, &minorVersion );
	eglBindAPI( EGL_OPENGL_API );
/*
	eglGetConfigs( mEGLDisplay, nullptr, 0, &count );
	EGLConfig* configs = (EGLConfig*)malloc( count * sizeof(count) );
	eglChooseConfig( mEGLDisplay, attribList, configs, count, &numConfigs );
	printf( "COUNT : %d\n", count );
	for ( int i = 0; i < count; i++ ) {
		EGLint id;
		eglGetConfigAttrib( mEGLDisplay, configs[i], EGL_NATIVE_VISUAL_ID, &id );
		printf( "CONFIG %d, %d\n", i, id );
	}
	for ( int i = 0; i < count; i++ ) {
		EGLint id;
		if ( !eglGetConfigAttrib( mEGLDisplay, configs[i], EGL_NATIVE_VISUAL_ID, &id ) ) {
			continue;
		}
		if ( id == GBM_FORMAT_ARGB8888 ) {
			mEGLConfig = configs[i];
			break;
		}
	}
	printf( "mEGLConfig : %p\n", mEGLConfig );
*/

	eglChooseConfig( mEGLDisplay, attribList, &mEGLConfig, 1, &numConfigs );
	mEGLContext = eglCreateContext( mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, contextAttribs );

	const EGLint egl_surface_attribs[] = {
		EGL_RENDER_BUFFER, EGL_BACK_BUFFER/* + 1 => disable double-buffer*/,
		EGL_NONE,
	};
#ifdef VARIANT_4
	mEGLSurface = eglCreateWindowSurface( mEGLDisplay, mEGLConfig, mGbmSurface, egl_surface_attribs );
#else
	mEGLSurface = eglCreateWindowSurface( mEGLDisplay, mEGLConfig, reinterpret_cast< EGLNativeWindowType >( &mLayerDisplay ), egl_surface_attribs );
#endif
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_WIDTH, (EGLint*)&mWidth );
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_HEIGHT, (EGLint*)&mHeight );
	eglMakeCurrent( mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext );

	std::cout << "OpenGL version : " << glGetString( GL_VERSION ) << "\n";
	std::cout << "Framebuffer resolution : " << mWidth << " x " << mHeight << "\n";
	glViewport( 0, 0, mWidth, mHeight );
// 	glViewport( 0, 0, OPTIMUM_WIDTH, OPTIMUM_HEIGHT );
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CCW );
	glCullFace( GL_BACK );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

	return 0;
}

// TODO / TBD : keep framebuffers generated by drmModeAddFB2() to prevent constant re-creation


void GLContext::SwapBuffers()
{
	eglSwapBuffers( mEGLDisplay, mEGLSurface );

#ifdef VARIANT_4
	struct gbm_bo* bo = gbm_surface_lock_front_buffer( mGbmSurface );
	uint32_t handle = gbm_bo_get_handle(bo).u32;
	uint32_t pitch = gbm_bo_get_stride(bo);
	uint32_t fb;
	const uint32_t h[4] = { handle, handle, handle, handle };
	const uint32_t p[4] = { pitch, pitch, pitch, pitch };
	const uint32_t o[4] = { 0, 0, 0, 0 };

	drmModeAddFB2( mDevice, mMode.hdisplay, mMode.vdisplay, GBM_FORMAT_ARGB8888, h, p, o, &fb, 0 );
// 	drmModeAddFB( mDevice, mMode.hdisplay, mMode.vdisplay, 24, 32, pitch, handle, &fb );
	drmModeSetCrtc( mDevice, mCrtc->crtc_id, fb, 0, 0, &mConnectorId, 1, &mMode );
	drmModeSetPlane( mDevice, mPlaneId, mCrtc->crtc_id, fb, 0, 0, 0, mMode.hdisplay, mMode.vdisplay, 0, 0, ((uint16_t)mMode.hdisplay) << 16, ((uint16_t)mMode.vdisplay) << 16 );

	if ( mPreviousBo ) {
		drmModeRmFB( mDevice, mPreviousFb );
		gbm_surface_release_buffer( mGbmSurface, mPreviousBo );
	}
	mPreviousBo = bo;
	mPreviousFb = fb;
#endif
}


#ifndef VARIANT_4
EGL_DISPMANX_WINDOW_T GLContext::CreateNativeWindow( int layer )
{
	EGL_DISPMANX_WINDOW_T nativewindow;
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	uint32_t display_width;
	uint32_t display_height;

	int ret = graphics_get_display_size( 5, &display_width, &display_height );
	std::cout << "display size ( " << ret << " ) : " << display_width << " x "<< display_height << "\n";

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = display_width;
	dst_rect.height = display_height;

	// Reduce resolution for better performances (automatically upscaled by dispman)
	display_width = OPTIMUM_WIDTH;
	display_height = OPTIMUM_HEIGHT;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = display_width << 16;
	src_rect.height = display_height << 16;

	mDisplay = vc_dispmanx_display_open( 5 );
	dispman_update = vc_dispmanx_update_start( 0 );

	VC_DISPMANX_ALPHA_T alpha;
	alpha.flags = (DISPMANX_FLAGS_ALPHA_T)( DISPMANX_FLAGS_ALPHA_FROM_SOURCE );
	alpha.opacity = 0;

	dispman_element = vc_dispmanx_element_add( dispman_update, mDisplay, layer, &dst_rect, 0, &src_rect, DISPMANX_PROTECTION_NONE, &alpha, 0, DISPMANX_NO_ROTATE );
	nativewindow.element = dispman_element;
	nativewindow.width = display_width;
	nativewindow.height = display_height;
	vc_dispmanx_update_submit_sync( dispman_update );

// 	mScreenshot.image = new uint8_t[ 1920 / 2 * 1080 * 3 ];
// 	mScreenshot.resource = vc_dispmanx_resource_create( VC_IMAGE_RGB888, 1920 / 2, 1080, &mScreenshot.vc_image_ptr );
// 	vc_dispmanx_rect_set( &mScreenshot.rect, 0, 0, 1920 / 2, 1080 );

	return nativewindow;
}
#endif


uint32_t GLContext::glWidth()
{
	return mWidth;
}


uint32_t GLContext::glHeight()
{
	return mHeight;
}


uint32_t GLContext::displayWidth()
{
	uint32_t display_width;
	uint32_t display_height;
	graphics_get_display_size( 5, &display_width, &display_height );
	(void)display_height;
	return display_width;
}


uint32_t GLContext::displayHeight()
{
	uint32_t display_width;
	uint32_t display_height;
	graphics_get_display_size( 5, &display_width, &display_height );
	(void)display_width;
	return display_height;
}


uint32_t GLContext::displayFrameRate()
{
	TV_DISPLAY_STATE_T tvstate;
	if ( vc_tv_get_display_state( &tvstate ) == 0 ) {
		HDMI_PROPERTY_PARAM_T property;
		property.property = HDMI_PROPERTY_PIXEL_CLOCK_TYPE;
		vc_tv_hdmi_get_property(&property);
		float frame_rate = ( property.param1 == HDMI_PIXEL_CLOCK_TYPE_NTSC ) ? tvstate.display.hdmi.frame_rate * (1000.0f/1001.0f) : tvstate.display.hdmi.frame_rate;
		return (uint32_t)frame_rate;
	}
	return 0;
}
