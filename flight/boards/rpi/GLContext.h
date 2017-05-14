#ifndef GLCONTEXT_H
#define GLCONTEXT_H

#include <vc_dispmanx_types.h>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

class GLContext
{
public:
	GLContext();
	~GLContext();

	int32_t Initialize( uint32_t width, uint32_t height );
	void SwapBuffers();

	static uint32_t displayWidth();
	static uint32_t displayHeight();
	static uint32_t displayFrameRate();

protected:
	EGL_DISPMANX_WINDOW_T CreateNativeWindow( int layer );

	uint32_t mWidth;
	uint32_t mHeight;

	EGL_DISPMANX_WINDOW_T mLayerDisplay;
	EGLDisplay mEGLDisplay;
	EGLConfig mEGLConfig;
	EGLContext mEGLContext;
	EGLSurface mEGLSurface;
	DISPMANX_DISPLAY_HANDLE_T mDisplay;
};

#endif // GLCONTEXT_H
