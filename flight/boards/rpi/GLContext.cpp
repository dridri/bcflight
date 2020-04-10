#include <iostream>
#include "GLContext.h"
#include <vc_dispmanx_types.h>
#include <bcm_host.h>

#define OPTIMUM_WIDTH 840
#define OPTIMUM_HEIGHT 480

GLContext::GLContext()
	: mWidth( 0 )
	, mHeight( 0 )
{
}


GLContext::~GLContext()
{
}


int32_t GLContext::Initialize( uint32_t width, uint32_t height )
{
	mLayerDisplay = CreateNativeWindow( 2 );

	EGLint attribList[] =
	{
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_RED_SIZE,          8,
		EGL_GREEN_SIZE,        8,
		EGL_BLUE_SIZE,         8,
		EGL_ALPHA_SIZE,        8,
		EGL_DEPTH_SIZE,        16,
		EGL_STENCIL_SIZE,      0,
		EGL_NONE
	};

	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	mEGLDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	eglInitialize( mEGLDisplay, &majorVersion, &minorVersion );
	eglGetConfigs( mEGLDisplay, nullptr, 0, &numConfigs );
	eglChooseConfig( mEGLDisplay, attribList, &mEGLConfig, 1, &numConfigs );
	mEGLContext = eglCreateContext( mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, contextAttribs );

	const EGLint egl_surface_attribs[] = {
		EGL_RENDER_BUFFER, EGL_BACK_BUFFER/* + 1 => disable double-buffer*/,
		EGL_NONE,
	};
	mEGLSurface = eglCreateWindowSurface( mEGLDisplay, mEGLConfig, reinterpret_cast< EGLNativeWindowType >( &mLayerDisplay ), egl_surface_attribs );
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_WIDTH, (EGLint*)&mWidth );
	eglQuerySurface( mEGLDisplay, mEGLSurface, EGL_HEIGHT, (EGLint*)&mHeight );
	eglMakeCurrent( mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext );

	std::cout << "OpenGL version : " << glGetString( GL_VERSION ) << "\n";
	std::cout << "Framebuffer resolution : " << mWidth << " x " << mHeight << "\n";
	glViewport( 0, 0, mWidth, mHeight );
	glViewport( 0, 0, OPTIMUM_WIDTH, OPTIMUM_HEIGHT );
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
	eglSwapBuffers( mEGLDisplay, mEGLSurface );
}


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
