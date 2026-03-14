#ifndef GLCONTEXTBACKEND_H
#define GLCONTEXTBACKEND_H

#include <cstdint>

typedef void* EGLDisplay;
typedef void* EGLConfig;
typedef void* EGLSurface;

class GLContextBackend {
public:
	virtual ~GLContextBackend() = default;
	virtual EGLDisplay createDisplay() = 0;
	virtual EGLSurface createSurface( EGLDisplay display, EGLConfig config, uint32_t width, uint32_t height ) = 0;
	virtual void postSwap( EGLDisplay display, EGLSurface surface ) = 0;
	virtual uint32_t displayWidth() = 0;
	virtual uint32_t displayHeight() = 0;
	virtual uint32_t displayFrameRate() = 0;
};

#endif // GLCONTEXTBACKEND_H
