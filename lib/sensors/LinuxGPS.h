#ifndef LINUXGPS_H

#define LINUXGPS_H

#ifdef BUILD_LinuxGPS
#include <gps.h>
#endif
#include "GPS.h"
#include "Lua.h"

LUA_CLASS class LinuxGPS : public GPS
{
public:
	LUA_EXPORT LinuxGPS();
	~LinuxGPS();

	virtual void Calibrate( float dt, bool last_pass );
	virtual time_t getTime();
	virtual bool Read( float* latitude, float* longitude, float* altitude, float* speed );
	virtual bool Stats( uint32_t* satSeen, uint32_t* satUsed );

protected:
#ifdef BUILD_LinuxGPS
	struct gps_data_t* mGpsData;
#endif
};


#endif // LINUXGPS_H
