#include <iostream>
#include <algorithm>
#include "Debug.h"
#include "GLContext.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include "Board.h"

#ifdef HAVE_DRM
#include "GLContextBackendDRM.h"
#endif

#ifdef HAVE_X11
#include "GLContextBackendX11.h"
#endif

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
	, mBackend( nullptr )
	, mReady( false )
	, mWidth( 0 )
	, mHeight( 0 )
	, mSyncLastTick( 0 )
{
#if defined(HAVE_X11) && defined(HAVE_DRM)
	if ( getenv( "DISPLAY" ) ) {
		mBackend = new GLContextBackendX11();
	} else {
		mBackend = new GLContextBackendDRM();
	}
#elif defined(HAVE_X11)
	mBackend = new GLContextBackendX11();
#elif defined(HAVE_DRM)
	mBackend = new GLContextBackendDRM();
#else
	#error "No GLContext backend available (need HAVE_DRM or HAVE_X11)"
#endif
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


int32_t GLContext::Initialize( uint32_t width, uint32_t height )
{
	EGLint attribList[] =
	{
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_RED_SIZE,          8,
		EGL_GREEN_SIZE,        8,
		EGL_BLUE_SIZE,         8,
		EGL_ALPHA_SIZE,        8,
		EGL_DEPTH_SIZE,        16,
		EGL_STENCIL_SIZE,      0,
		EGL_SAMPLE_BUFFERS,    1,
		EGL_SAMPLES,           4,
		EGL_NONE
	};

	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	mEGLDisplay = mBackend->createDisplay();

	eglInitialize( mEGLDisplay, &majorVersion, &minorVersion );
	eglBindAPI( EGL_OPENGL_ES_API );

	eglChooseConfig( mEGLDisplay, attribList, &mEGLConfig, 1, &numConfigs );
	mEGLContext = eglCreateContext( mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, contextAttribs );

	mEGLSurface = mBackend->createSurface( mEGLDisplay, mEGLConfig, width, height );
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_WIDTH,  (EGLint*)&mWidth );
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_HEIGHT, (EGLint*)&mHeight );
	if ( mWidth == 0 )  { mWidth  = width; }
	if ( mHeight == 0 ) { mHeight = height; }
	eglMakeCurrent( mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext );

	eglSwapInterval( mEGLDisplay, 1 );

	std::cout << "OpenGL version : " << glGetString( GL_VERSION ) << "\n";
	std::cout << "Framebuffer resolution : " << mWidth << " x " << mHeight << "\n";
	glViewport( 0, 0, mWidth, mHeight );
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CCW );
	glCullFace( GL_BACK );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

	return 0;
}


void GLContext::SwapBuffers()
{
	glFinish();
	eglSwapBuffers( mEGLDisplay, mEGLSurface );
	mBackend->postSwap( mEGLDisplay, mEGLSurface );
	mSyncLastTick = Board::WaitTick( 1000000L / 30, mSyncLastTick );
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
	return sInstance ? sInstance->mBackend->displayWidth() : 0;
}


uint32_t GLContext::displayHeight()
{
	return sInstance ? sInstance->mBackend->displayHeight() : 0;
}


uint32_t GLContext::displayFrameRate()
{
	return sInstance ? sInstance->mBackend->displayFrameRate() : 0;
}
