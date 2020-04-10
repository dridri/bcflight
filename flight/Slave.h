#ifndef SLAVE_H
#define SLAVE_H

#include <string>
#include "Bus.h"

class Config;
class Thread;

using namespace STD;

class Slave
{
public:
	typedef enum {
		UNKNOWN = 0x00,
		CONFIG_LEN = 0x01,
		CONFIG = 0x02,
		MOTOR_DISABLE = 0x10, // [u8]motor_id
		MOTOR_DISARM = 0x11, // [u8]motor_id
		MOTOR_SET = 0x13, // [u8]motor_id, [u16]speed (setting speed to 0 means arming)
	} Cmd;


	Slave( Config* config, const string& object, bool is_master = false );
	~Slave();

	// Master side
	void Configure( string& conf );

	// Slave side

protected:
	Bus* mBus;

	int32_t Write( Cmd cmd, const uint8_t* data, uint32_t len );

	// Slave-only
	Thread* mThread;
	bool run();
};


#ifdef _IN_SLAVE_IMPL
static const uint8_t cmds_sizes[] = {
	0,     // UNKNOWN
	4,     // CONFIG_LEN
	0xFF,  // CONFIG (variable)
	1,     // MOTOR_DISABLE
	1,     // MOTOR_DISARM
	1+2,   // MOTOR_SET
};
#endif // _IN_SLAVE_IMPL


#endif // SLAVE_H
