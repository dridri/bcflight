#ifndef DRMSURFACE_H
#define DRMSURFACE_H

#include <stdint.h>
#include "DRM.h"
#include "DRMFrameBuffer.h"

typedef struct _drmModeModeInfo drmModeModeInfo;


class DRMSurface : public DRM
{
public:
	DRMSurface( int32_t zindex = 0 );
	~DRMSurface();

	void Show( DRMFrameBuffer* fb );

	const drmModeModeInfo* mode() const;

protected:
	uint32_t mConnectorId;
	uint32_t mCrtcId;
	uint32_t mPlaneId;
	drmModeModeInfo* mMode;
	int32_t mZindex;
	bool mKmsSet;
};

#endif // DRMSURFACE_H
