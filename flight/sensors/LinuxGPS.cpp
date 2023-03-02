#if ( defined( BOARD_rpi ) )

#include "LinuxGPS.h"
#include "Debug.h"
#include <cmath>


LinuxGPS::LinuxGPS()
	: GPS()
	, mGpsData( nullptr )
{
	int rc = 0;
	mGpsData = new struct gps_data_t;
	memset( mGpsData, 0, sizeof(struct gps_data_t) );
	if ( ( rc = gps_open( "localhost", "2947", mGpsData ) ) < 0 ) {
		gError() << "gps_open failed : " << gps_errstr(rc) << " (" << rc << ")";
		delete mGpsData;
		mGpsData = nullptr;
		return;
	}
	gps_stream( mGpsData, WATCH_ENABLE | WATCH_JSON, nullptr );
}


LinuxGPS::~LinuxGPS()
{
	if ( mGpsData ) {
		gps_stream( mGpsData, WATCH_DISABLE, nullptr );
		gps_close( mGpsData );
	}
}


void LinuxGPS::Calibrate( float dt, bool last_pass )
{
	(void)dt;
	(void)last_pass;
}


void LinuxGPS::Read( float* latitude, float* longitude, float* altitude, float* speed )
{
	int rc = 0;
	if ( mGpsData and gps_waiting( mGpsData, 0 ) ) {
		if ( ( rc = gps_read( mGpsData, nullptr, 0 ) ) < 0 ) {
			gDebug() << "gps_read failed : " << gps_errstr(rc) << " (" << rc << ")";
		} else {
			if ( mGpsData->set & MODE_SET ) {
				if ( mGpsData->fix.mode < 0 || mGpsData->fix.mode >= 3 ) {
					mGpsData->fix.mode = 0;
				}
				if ( ( mGpsData->fix.mode == MODE_2D or mGpsData->fix.mode == MODE_3D ) and not isnan(mGpsData->fix.latitude) and not isnan(mGpsData->fix.longitude) ) {
					gDebug() << "latitude: " << mGpsData->fix.latitude << ", longitude: " << mGpsData->fix.longitude << ", speed: " << mGpsData->fix.speed << ", timestamp: " << mGpsData->fix.time.tv_sec;
					*latitude = mGpsData->fix.latitude;
					*longitude = mGpsData->fix.longitude;
					*altitude = mGpsData->fix.altitude;
					*speed = mGpsData->fix.speed;
					
				} else {
					gDebug() << "no GPS data available";
				}
			}
		}
	}
}

#endif // BOARD_rpi
