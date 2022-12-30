#ifndef DRMFRAMEBUFFER_H
#define DRMFRAMEBUFFER_H

#include <stdint.h>
#include <drm_fourcc.h>
#include "DRM.h"

class DRMFrameBuffer : public DRM
{
public:
	DRMFrameBuffer( uint32_t width, uint32_t height, uint32_t stride, uint32_t format = DRM_FORMAT_YUV420, uint32_t bufferObjectHandle = 0, int dmaFd = -1 );
	~DRMFrameBuffer();

	const uint32_t handle() const;
	const uint32_t width() const;
	const uint32_t height() const;

protected:
	uint32_t mWidth;
	uint32_t mHeight;
	int mDmaFd;
	uint32_t mBufferObjectHandle;
	uint32_t mFrameBufferHandle;
};

#endif // DRMFRAMEBUFFER_H
