#ifndef GLCONTEXT_H
#define GLCONTEXT_H

#include <bcm_host.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <vector>
#include <list>
#include <map>
#include <functional>
#include <mutex>
#include "Thread.h"
#include "DRMFrameBuffer.h"
#include "DRMSurface.h"

class GLContext : public Thread
{
public:
	GLContext();
	~GLContext();

	int32_t Initialize( uint32_t width, uint32_t height );
	void SwapBuffers();

	uint32_t addLayer( int32_t x, int32_t y, int32_t width, int32_t height, GLenum format = GL_RGBA, int32_t layerIndex = 0 );
	uint32_t addLayer( const std::function<void()>& f, int32_t layerIndex = 0 );

	uint32_t glWidth();
	uint32_t glHeight();
	static uint32_t displayWidth();
	static uint32_t displayHeight();
	static uint32_t displayFrameRate();

	EGLImageKHR eglImage( uint32_t glImage );
	const EGLDisplay eglDisplay() const;
	const EGLContext eglContext() const;

	void runOnGLThread( const std::function<void()>& f, bool wait = false );

	static GLContext* instance();

protected:
	static GLContext* sInstance;

	virtual bool run();

	typedef struct {
		int32_t index;
		std::function<void()> fct;
		int32_t x;
		int32_t y;
		uint32_t width;
		uint32_t height;
		uint32_t glId;
	} Layer;

	bool mReady;
	uint32_t mWidth;
	uint32_t mHeight;
	std::vector<Layer> mLayers;
	std::mutex mLayersMutex;
	std::list<std::function<void()>> mContextQueue;
	std::mutex mContextQueueMutex;

	std::map< struct gbm_bo*, DRMFrameBuffer* > mFrameBuffers;
	DRMSurface* mRenderSurface;

	struct gbm_device* mGbmDevice;
	struct gbm_surface* mGbmSurface;
	struct gbm_bo* mPreviousBo;
	EGLConfig getDisplay();

	EGLDisplay mEGLDisplay;
	EGLConfig mEGLConfig;
	EGLContext mEGLContext;
	EGLSurface mEGLSurface;
	uint64_t mSyncLastTick;
};

#endif // GLCONTEXT_H
