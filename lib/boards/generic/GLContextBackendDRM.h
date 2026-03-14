#ifndef GLCONTEXTBACKENDDRM_H
#define GLCONTEXTBACKENDDRM_H

#include <map>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include "GLContextBackend.h"
#include "DRM.h"
#include "DRMSurface.h"
#include "DRMFrameBuffer.h"

class GLContextBackendDRM : public GLContextBackend {
public:
	GLContextBackendDRM()
		: mGbmDevice( nullptr )
		, mGbmSurface( nullptr )
		, mRenderSurface( nullptr )
		, mPreviousBo( nullptr )
	{}

	EGLDisplay createDisplay() override {
		mRenderSurface = new DRMSurface( 100 );
		mGbmDevice = gbm_create_device( DRM::drmFd() );
		mGbmSurface = gbm_surface_create(
			mGbmDevice,
			mRenderSurface->mode()->hdisplay,
			mRenderSurface->mode()->vdisplay,
			GBM_FORMAT_ARGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING );
		return eglGetDisplay( reinterpret_cast<EGLNativeDisplayType>( mGbmDevice ) );
	}

	EGLSurface createSurface( EGLDisplay display, EGLConfig config, uint32_t /*width*/, uint32_t /*height*/ ) override {
		const EGLint attribs[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE };
		return eglCreateWindowSurface(
			display, config,
			reinterpret_cast<EGLNativeWindowType>( mGbmSurface ),
			attribs );
	}

	void postSwap( EGLDisplay /*display*/, EGLSurface /*surface*/ ) override {
		struct gbm_bo* bo = gbm_surface_lock_front_buffer( mGbmSurface );
		auto iter = mFrameBuffers.find( bo );
		DRMFrameBuffer* fb;
		if ( iter == mFrameBuffers.end() ) {
			uint32_t handle = gbm_bo_get_handle( bo ).u32;
			uint32_t width  = gbm_bo_get_width( bo );
			uint32_t height = gbm_bo_get_height( bo );
			uint32_t stride = gbm_bo_get_stride( bo );
			fb = new DRMFrameBuffer( width, height, stride, GBM_FORMAT_ARGB8888, handle );
			mFrameBuffers.emplace( bo, fb );
		} else {
			fb = iter->second;
		}
		mRenderSurface->Show( fb );
		if ( mPreviousBo ) {
			gbm_surface_release_buffer( mGbmSurface, mPreviousBo );
		}
		mPreviousBo = bo;
	}

	uint32_t displayWidth()     override { return mRenderSurface ? mRenderSurface->mode()->hdisplay : 0; }
	uint32_t displayHeight()    override { return mRenderSurface ? mRenderSurface->mode()->vdisplay : 0; }
	uint32_t displayFrameRate() override { return mRenderSurface ? mRenderSurface->mode()->vrefresh : 0; }

private:
	struct gbm_device*  mGbmDevice;
	struct gbm_surface* mGbmSurface;
	DRMSurface*         mRenderSurface;
	struct gbm_bo*      mPreviousBo;
	std::map<struct gbm_bo*, DRMFrameBuffer*> mFrameBuffers;
};

#endif // GLCONTEXTBACKENDDRM_H
