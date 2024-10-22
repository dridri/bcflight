#include <xf86drm.h>
#include <xf86drmMode.h>
#include <string.h>
#include <string>
#include "DRMFrameBuffer.h"
#include "Debug.h"
#include <drm_fourcc.h>


DRMFrameBuffer::DRMFrameBuffer( uint32_t width, uint32_t height, uint32_t stride, uint32_t format, uint32_t bufferObjectHandle, int dmaFd )
	: mWidth( width )
	, mHeight( height )
	, mDmaFd( dmaFd )
	, mBufferObjectHandle( bufferObjectHandle )
	, mFrameBufferHandle( 0 )
{
	fDebug( width, height, stride, format, bufferObjectHandle, dmaFd );

	if ( !bufferObjectHandle and dmaFd ) {
		if ( drmPrimeFDToHandle( drmFd(), dmaFd, &mBufferObjectHandle ) ) {
			gError() << "drmPrimeFDToHandle failed for fd " << dmaFd << " : " << strerror(errno);
		}
	}

	// uint32_t offsets[4] = { 0 };
	// uint32_t pitches[4] = { 0 };
	// uint32_t bo_handles[4] = { 0 };

	uint32_t offsets[4] = {
		0,
		stride * height,
		stride * height + (stride / 2) * (height / 2)
		
	};
	uint32_t pitches[4] = {
		stride,
		stride / 2,
		stride / 2
		
	};
	uint32_t bo_handles[4] = { mBufferObjectHandle, mBufferObjectHandle, mBufferObjectHandle };

	if ( format == DRM_FORMAT_YUV420 ) {
		offsets[1] = stride * height;
		offsets[2] = stride * height + (stride / 2) * (height / 2);
		pitches[0] = stride;
		pitches[1] = stride / 2;
		pitches[2] = stride / 2;
		bo_handles[0] = mBufferObjectHandle;
		bo_handles[1] = mBufferObjectHandle;
		bo_handles[2] = mBufferObjectHandle;
	} else if ( format == DRM_FORMAT_ARGB8888 ) {
		pitches[0] = stride;
		pitches[1] = stride;
		pitches[2] = stride;
		pitches[3] = stride;
		bo_handles[0] = mBufferObjectHandle;
		bo_handles[1] = mBufferObjectHandle;
		bo_handles[2] = mBufferObjectHandle;
		bo_handles[3] = mBufferObjectHandle;
	}

	if ( drmModeAddFB2( drmFd(), width, height, format, bo_handles, pitches, offsets, &mFrameBufferHandle, 0 ) ) {
		gError() << "drmModeAddFB2 failed : " << strerror(errno);
	}
}


DRMFrameBuffer::~DRMFrameBuffer()
{
}


const uint32_t DRMFrameBuffer::handle() const
{
	return mFrameBufferHandle;
}


const uint32_t DRMFrameBuffer::width() const
{
	return mWidth;
}


const uint32_t DRMFrameBuffer::height() const
{
	return mHeight;
}

