#ifndef DRMFRAMEBUFFER_H
#define DRMFRAMEBUFFER_H

#include <stdint.h>
#include "DRM.h"

#define __fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

class DRMFrameBuffer : public DRM
{
public:
	DRMFrameBuffer( uint32_t width, uint32_t height, uint32_t stride, uint32_t format = __fourcc_code('Y', 'U', '1', '2')/*DRM_FORMAT_YUV420*/, uint32_t bufferObjectHandle = 0, int dmaFd = -1 );
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
