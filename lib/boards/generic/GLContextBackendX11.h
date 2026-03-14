#ifndef GLCONTEXTBACKENDX11_H
#define GLCONTEXTBACKENDX11_H

#include <stdexcept>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include "GLContextBackend.h"

class GLContextBackendX11 : public GLContextBackend {
public:
	GLContextBackendX11() : mXDisplay( nullptr ), mXWindow( 0 ) {}

	EGLDisplay createDisplay() override {
		mXDisplay = XOpenDisplay( nullptr );
		if ( !mXDisplay ) {
			throw std::runtime_error( "GLContextBackendX11: XOpenDisplay failed" );
		}
		return eglGetDisplay( reinterpret_cast<EGLNativeDisplayType>( mXDisplay ) );
	}

	EGLSurface createSurface( EGLDisplay display, EGLConfig config, uint32_t width, uint32_t height ) override {
		int screen = DefaultScreen( mXDisplay );

		EGLint visualId = 0;
		eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &visualId );

		XVisualInfo vTemplate = {};
		vTemplate.visualid = (VisualID)visualId;
		int nVisuals = 0;
		XVisualInfo* vi = XGetVisualInfo( mXDisplay, VisualIDMask, &vTemplate, &nVisuals );

		XSetWindowAttributes swa = {};
		swa.colormap  = XCreateColormap( mXDisplay, RootWindow( mXDisplay, screen ), vi->visual, AllocNone );
		swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

		uint32_t w = width;
		uint32_t h = height;
		mXWindow = XCreateWindow(
			mXDisplay, RootWindow( mXDisplay, screen ),
			0, 0, w, h,
			0, vi->depth, InputOutput, vi->visual,
			CWColormap | CWEventMask, &swa );
		XMapWindow( mXDisplay, mXWindow );
		XFree( vi );

		const EGLint attribs[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE };
		return eglCreateWindowSurface(
			display, config,
			reinterpret_cast<EGLNativeWindowType>( mXWindow ),
			attribs );
	}

	void postSwap( EGLDisplay /*display*/, EGLSurface /*surface*/ ) override {
		// X11 compositor handles presentation after eglSwapBuffers — nothing to do
	}

	uint32_t displayWidth()     override { return mXDisplay ? (uint32_t)DisplayWidth( mXDisplay, DefaultScreen( mXDisplay ) )  : 0; }
	uint32_t displayHeight()    override { return mXDisplay ? (uint32_t)DisplayHeight( mXDisplay, DefaultScreen( mXDisplay ) ) : 0; }
	uint32_t displayFrameRate() override { return 60; }

private:
	Display* mXDisplay;
	Window   mXWindow;
};

#endif // GLCONTEXTBACKENDX11_H
