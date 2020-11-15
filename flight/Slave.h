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

		STAB_ROLL_P = 0x60,
		STAB_ROLL_I = 0x61,
		STAB_ROLL_D = 0x62,
		STAB_PITCH_P = 0x63,
		STAB_PITCH_I = 0x64,
		STAB_PITCH_D = 0x65,
		STAB_YAW_P = 0x66,
		STAB_YAW_I = 0x67,
		STAB_YAW_D = 0x68,
		STAB_OUTER_P = 0x69,
		STAB_OUTER_I = 0x6a,
		STAB_OUTER_D = 0x6b,
		STAB_HORIZ_OFFSET = 0x6c,
		STAB_MODE = 0x6d,
		STAB_ALTI_HOLD = 0x6e,
		STAB_ARM = 0x6f,
		STAB_ROLL = 0x70,
		STAB_PITCH = 0x71,
		STAB_YAW = 0x72,
		STAB_THRUST = 0x73,
		STAB_CALIBRATE_ESCS = 0x74,
		STAB_RESET = 0x75,
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
