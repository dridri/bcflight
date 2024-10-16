#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>

#include <string>

#include "DRM.h"
#include "Debug.h"

int DRM::sDrmFd = -1;


int DRM::drmFd()
{
	if ( sDrmFd < 0 ) {
		for ( int dev = 0; dev <= 1; dev++ ) {
			std::string device = std::string("/dev/dri/card") + std::to_string(dev);
			sDrmFd = open( device.c_str(), O_RDWR/* | O_CLOEXEC*/ );
			if ( sDrmFd >= 0 ) {
				drmModeResPtr res = drmModeGetResources( sDrmFd );
				if ( !res ) {
					close( sDrmFd );
					sDrmFd = -1;
				} else {
					drmModeFreeResources( res );
					gDebug() << "Card " << device << " has DRM capability";
					break;
				}
			}
		}

		if ( sDrmFd < 0 ) {
			gError() << "No /dev/dri/cardX available with DRM capability (" << strerror(errno) << ")";
			exit(3);
		}

		drmSetMaster( sDrmFd );
	}

	return sDrmFd;
}
