#ifndef LINUXGPS_H
#define LINUXGPS_H

#if ( defined( BOARD_rpi ) )

#include <gps.h>
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
	struct gps_data_t* mGpsData;
};



#endif // BOARD_rpi

#endif // LINUXGPS_H
