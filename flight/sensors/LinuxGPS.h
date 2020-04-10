#ifndef LINUXGPS_H
#define LINUXGPS_H

#if ( defined( BOARD_rpi ) )
/*

#include "GPS.h"
#include <gps.h>

class LinuxGPS : public GPS
{
public:
	LinuxGPS();
	~LinuxGPS();

	virtual void Calibrate( float dt, bool last_pass );
	virtual void Read( float* lattitude, float* longitude, float* altitude, float* speed );

	static Sensor* Instanciate( Config* config, const string& object );
	static int flight_register( Main* main );

protected:
	struct gps_data_t* mGpsData;
};


*/

class Main;
class LinuxGPS
{
public:
	static int flight_register( Main* main ) { return 0; }
};

#else

class LinuxGPS {
public:
	static int flight_register( Main* main ) { return 0; }
};

#endif // BOARD_rpi

#endif // LINUXGPS_H
