/*
#if ( defined( BOARD_rpi ) )

#include "LinuxGPS.h"
#include "Debug.h"
#include <cmath>

int LinuxGPS::flight_register( Main* main )
{
	Device dev;
	dev.iI2CAddr = 0;
	dev.name = "LinuxGPS";
	dev.fInstanciate = LinuxGPS::Instanciate;
	mKnownDevices.push_back( dev );
	return 0;
}


Sensor* LinuxGPS::Instanciate( Config* config, const string& object )
{
	int rc = 0;
	struct gps_data_t gps_data;
	if ( ( rc = gps_open( "localhost", "2947", &gps_data ) ) < 0 ) {
		return nullptr;
	}
	gps_close ( &gps_data );
	return new LinuxGPS();
}

LinuxGPS::LinuxGPS()
	: GPS()
	, mGpsData( nullptr )
{
	int rc = 0;
	if ( ( rc = gps_open( "localhost", "2947", mGpsData ) ) < 0 ) {
		gDebug() << "gps_open failed : " << gps_errstr(rc) << " (" << rc << ")\n";
		mGpsData = nullptr;
		return;
	}
	gps_stream( mGpsData, WATCH_ENABLE | WATCH_JSON, nullptr );
}


LinuxGPS::~LinuxGPS()
{
	gps_stream( mGpsData, WATCH_DISABLE, nullptr );
	gps_close( mGpsData );
}


void LinuxGPS::Calibrate( float dt, bool last_pass )
{
	if ( mGpsData == nullptr ) {
		return;
	}
	// Nothing to do ?
}


void LinuxGPS::Read( float* lattitude, float* longitude, float* altitude, float* speed )
{
	int rc = 0;
	if ( gps_waiting( mGpsData, 1 ) ) {
		if ( ( rc = gps_read( mGpsData ) ) < 0 ) {
		gDebug() << "gps_read failed : " << gps_errstr(rc) << " (" << rc << ")\n";
		} else {
			if ( ( mGpsData->status == STATUS_FIX ) and ( mGpsData->fix.mode == MODE_2D or mGpsData->fix.mode == MODE_3D ) and not isnan(mGpsData->fix.latitude) and not isnan(mGpsData->fix.longitude) ) {
				gDebug() << "latitude: " << mGpsData->fix.latitude << ", longitude: " << mGpsData->fix.longitude << ", speed: " << mGpsData->fix.speed << ", timestamp: " << mGpsData->fix.time << "\n";
			} else {
				gDebug() << "no GPS data available\n";
			}
		}
	}
}

#endif // BOARD_rpi
*/