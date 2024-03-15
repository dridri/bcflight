#include <iostream>
#include <algorithm>
#include "Debug.h"
#include "GLContext.h"
// #include <vc_dispmanx_types.h>
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
	, mPreviousBo( nullptr )
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

	return 0;
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



EGLConfig GLContext::getDisplay()
{
	mRenderSurface = new DRMSurface( 100 );

	mGbmDevice = gbm_create_device( DRM::drmFd() );
	mGbmSurface = gbm_surface_create( mGbmDevice, mRenderSurface->mode()->hdisplay, mRenderSurface->mode()->vdisplay, GBM_FORMAT_ARGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING );

	return eglGetDisplay( reinterpret_cast<EGLNativeDisplayType>(mGbmDevice) );
}


int32_t GLContext::Initialize( uint32_t width, uint32_t height )
{
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

	mEGLDisplay = getDisplay();


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
	mEGLSurface = eglCreateWindowSurface( mEGLDisplay, mEGLConfig, reinterpret_cast<EGLNativeWindowType>(mGbmSurface), egl_surface_attribs );
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_WIDTH, (EGLint*)&mWidth );
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_HEIGHT, (EGLint*)&mHeight );
	eglMakeCurrent( mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext );

	eglSwapInterval( mEGLDisplay, 1 );

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
	glFlush();
	glFinish();
	usleep( 1000 * 1000 * 1 / 40 );
	eglSwapBuffers( mEGLDisplay, mEGLSurface );

	struct gbm_bo* bo = gbm_surface_lock_front_buffer( mGbmSurface );
	DRMFrameBuffer* fb = nullptr;
	auto iter = mFrameBuffers.find( bo );
	if ( iter == mFrameBuffers.end() ) {
		uint32_t handle = gbm_bo_get_handle(bo).u32;
		uint32_t width = gbm_bo_get_width(bo);
		uint32_t height = gbm_bo_get_height(bo);
		uint32_t stride = gbm_bo_get_stride(bo);
		fb = new DRMFrameBuffer( width, height, stride, GBM_FORMAT_ARGB8888, handle );
		mFrameBuffers.emplace( std::make_pair( bo, fb ) );
	} else {
		fb = (*iter).second;
	}

	mRenderSurface->Show( fb );

	if ( mPreviousBo ) {
		gbm_surface_release_buffer( mGbmSurface, mPreviousBo );
	}
	mPreviousBo = bo;
}


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
	return 0;
}


uint32_t GLContext::displayHeight()
{
	return 0;
}


uint32_t GLContext::displayFrameRate()
{
	return 0;
}
