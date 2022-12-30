#include <xf86drm.h>
#include <xf86drmMode.h>
#include <stdio.h>
#include <list>
#include <algorithm>

#include "DRMSurface.h"
#include "Debug.h"

std::list< uint32_t > usedPlanes;

DRMSurface::DRMSurface( int32_t zindex )
	: mZindex( zindex )
	, mKmsSet( false )
{
	drmModeRes* resources = drmModeGetResources( drmFd() );
	drmModeConnector* connector = nullptr;

	for ( int i = 0; i < resources->count_connectors; i++ ) {
		drmModeConnector* conn = drmModeGetConnector( drmFd(), resources->connectors[i] );
		if ( conn->connection == DRM_MODE_CONNECTED and ( conn->connector_type != DRM_MODE_CONNECTOR_DPI and conn->connector_type != DRM_MODE_CONNECTOR_DSI ) ) {
			connector = conn;
			break;
		} else if ( conn ) {
			drmModeFreeConnector( conn );
		}
	}
	if ( connector == nullptr ) {
		fprintf(stderr, "Unable to get connector\n");
		drmModeFreeResources(resources);
		exit(0);
	}

	mConnectorId = connector->connector_id;
	gDebug() << "Connector " << mConnectorId << " modes :";
	for ( int i = 0; i < connector->count_modes; i++ ) {
		gDebug() << i << " : " << connector->modes[i].hdisplay << "x" << connector->modes[i].vdisplay << "@" << connector->modes[i].vrefresh;
	}
	for ( int i = 0; i < connector->count_modes; i++ ) {
		// if ( i == 22 ) {
		// if ( connector->modes[i].hdisplay == 1280 and connector->modes[i].vdisplay == 720 ) {
			mMode = connector->modes[i];
			break;
		// }
	}
	// printf("resolution: %ix%i\n", mMode.hdisplay, mMode.vdisplay);

    drmModeEncoder* encoder = ( connector->encoder_id ? drmModeGetEncoder( drmFd(), connector->encoder_id ) : nullptr );
	if ( encoder == nullptr ) {
		fprintf(stderr, "Unable to get encoder\n");
		drmModeFreeConnector(connector);
		drmModeFreeResources(resources);
		exit(0);
	}

	mCrtcId = encoder->crtc_id;
	drmModeSetCrtc( drmFd(), mCrtcId, 0, 0, 0, &mConnectorId, 1, &mMode );

	mPlaneId = 0;
	drmModePlaneResPtr planes = drmModeGetPlaneResources( drmFd() );
	zindex = std::min( zindex, (int32_t)planes->count_planes - 1 );

	for ( int i = 0; i < planes->count_planes && mPlaneId == 0; i++ ) {
		drmModePlanePtr plane = drmModeGetPlane( drmFd(), planes->planes[i] );
		if ( i >= zindex and std::find(usedPlanes.begin(), usedPlanes.end(), plane->plane_id) == usedPlanes.end() ) {
			// printf( "Plane %d : %d %d %d %d\n", i, plane->x, plane->y, plane->crtc_x, plane->crtc_y );
			drmModeObjectProperties* props = drmModeObjectGetProperties( drmFd(), plane->plane_id, DRM_MODE_OBJECT_ANY );
			for ( int j = 0; j < props->count_props && mPlaneId == 0; j++ ) {
				drmModePropertyRes* prop = drmModeGetProperty( drmFd(), props->props[j] );
				// printf( " %s : %llu\n", prop->name, props->prop_values[j] );
				if ( strcmp(prop->name, "type") == 0 and props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY ) {
					mPlaneId = plane->plane_id;
					usedPlanes.push_back( mPlaneId );
					break;
				}
				drmModeFreeProperty(prop);
			}
			drmModeFreeObjectProperties(props);
		}
		drmModeFreePlane(plane);
	}
	// printf( " ================> plane_id : 0x%08X\n", mPlaneId );

	drmModeFreeEncoder(encoder);
	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);
}


DRMSurface::~DRMSurface()
{
}

const drmModeModeInfo* DRMSurface::mode() const
{
	return &mMode;
}


void DRMSurface::Show( DRMFrameBuffer* fb )
{
	// fDebug( drmFd(), mPlaneId, mCrtcId, fb->handle(), 0, 0, 0, fb->width(), fb->height(), 0, 0, mMode.hdisplay, mMode.vdisplay );
	// if ( drmModeSetPlane( drmFd(), mPlaneId, mCrtcId, fb->handle(), 0, 0, 0, fb->width(), fb->height(), 0, 0, mMode.hdisplay << 16, mMode.vdisplay << 16 ) ) {
	if ( mZindex == 0 and not mKmsSet ) {
		mKmsSet = true;
		drmModeSetCrtc( drmFd(), mCrtcId, fb->handle(), 0, 0, &mConnectorId, 1, &mMode );
	} else {
		if ( drmModeSetPlane( drmFd(), mPlaneId, mCrtcId, fb->handle(), 0, 0, 0, mMode.hdisplay, mMode.vdisplay, 0, 0, fb->width() << 16, fb->height() << 16 ) ) {
			throw std::runtime_error("drmModeSetPlane failed: " + std::string(strerror(errno)));
		}
	}
}

