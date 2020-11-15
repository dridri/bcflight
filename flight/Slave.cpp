#define _IN_SLAVE_IMPL
#include "Slave.h"
#include "Config.h"
#include "Thread.h"
#include "Debug.h"
#include "SPI.h"
#include "I2C.h"
#include "Main.h"
#include "Frame.h"


Slave::Slave( Config* config, const string& object, bool is_master )
	: mBus( nullptr )
{
	/*
	string bus = config->String( object + ".bus" );
	if ( bus.find("spi") != string::npos or bus.find("SPI") != string::npos ) {
		mBus = new SPI( bus, 0 );
	} else if ( bus.find("i2c") != string::npos or bus.find("I2C") != string::npos ) {
		mBus = new I2C( config->Integer( object + ".address" ), true );
	}

	if ( is_master ) {
		string conf = config->DumpVariable( object );
		uint32_t len = board_htonl( conf.length() );
		Write( CONFIG_LEN, (uint8_t*)&len, sizeof(uint32_t) );
		Write( CONFIG, (uint8_t*)conf.c_str(), len );
	} else {
		while ( 1 ) {
			char test = 0;
			int ret = mBus->Read( &test, 1 );
			if ( ret == 1 ) {
				printf( 0, "received 0x%02X %d '%c'\n", test, test, test );
			} else {
				gDebug() << "error\n";
			}
		}

		const string _test_conf = ""
			"loop_time = 30\n"
			"frame.motors = {\n"
			"	OneShot125 { pin = ( 3 << 4 ) | 12 },\n"
			"	OneShot125 { pin = ( 3 << 4 ) | 13 },\n"
			"	OneShot125 { pin = ( 3 << 4 ) | 14 },\n"
			"	OneShot125 { pin = ( 3 << 4 ) | 15 },\n"
			"}\n"
			"frame.motors[1].pid_vector = Vector( -1.0,  1.0, -1.0 )\n"
			"frame.motors[2].pid_vector = Vector(  1.0,  1.0,  1.0 )\n"
			"frame.motors[3].pid_vector = Vector( -1.0, -1.0,  1.0 )\n"
			"frame.motors[4].pid_vector = Vector(  1.0, -1.0, -1.0 )\n"
		"";
		config->Execute( _test_conf );
		gDebug() << config->DumpVariable( "frame" );

		uint8_t cmd = 0;
		uint32_t sz = 0;
		while ( 1 ) {
			if ( mBus->Read( &cmd, 1 ) == 1 and cmd == CONFIG_LEN ) {
				if ( mBus->Read( &sz, sizeof(sz) ) == sizeof(sz) ) {
					sz = board_ntohl( sz );
					uint8_t* buffer = (uint8_t*)malloc( sz + 1 );
					if ( mBus->Read( &cmd, 1 ) == 1 and cmd == CONFIG and (uint32_t)mBus->Read( buffer, sz ) == sz ) {
						buffer[sz] = '\0';
						Config* conf = Main::instance()->config();
						conf->Execute( (char*)buffer );
					}
					free( buffer );
					break;
				}
			}
		}

		mThread = new HookThread<Slave>( "slave", this, &Slave::run );
		mThread->setFrequency( 1000000 / config->Integer( "loop_time" ) );
		mThread->Start();
	}
	*/
}


Slave::~Slave()
{
}


int32_t Slave::Write( Cmd cmd, const uint8_t* data, uint32_t len )
{
	return mBus->Write( (uint8_t)cmd, data, len );
}


bool Slave::run()
{
	return false;
	/*
	uint8_t cmd = 0;
	uint8_t buf[128] = { 0 };

	if ( mBus->Read( &cmd, 1 ) == 1 and cmd > 0 ) {
		uint8_t cmd_size = cmds_sizes[cmd];
		if ( mBus->Read( buf, cmd_size ) == cmd_size ) {
			switch ( cmd ) {
				case MOTOR_DISABLE : {
					uint8_t imotor = ((uint8_t*)buf)[0];
					Main::instance()->frame()->motors()->at(imotor)->Disable();
					break;
				}
				case MOTOR_DISARM : {
					uint8_t imotor = ((uint8_t*)buf)[0];
					Main::instance()->frame()->motors()->at(imotor)->Disarm();
					break;
				}
				case MOTOR_SET : {
					uint8_t imotor = ((uint8_t*)buf)[0];
					uint16_t value = 0;
					memcpy( &value, (uint16_t*)( buf+1 ), sizeof(value) );
					float speed = (float)board_ntohs(value) / 65535.0f;
					Main::instance()->frame()->motors()->at(imotor)->setSpeed( speed );
					break;
				}
			}
		}
	}

	return true;
	*/
}
