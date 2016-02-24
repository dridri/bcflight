#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/signal.h>

extern "C" {
#include <interface/vmcs_host/vc_vchi_gencmd.h>
#include <interface/vmcs_host/vc_vchi_bufman.h>
#include <interface/vmcs_host/vc_tvservice.h>
#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vchiq_arm/vchiq_if.h>
};

#include <wiringPi.h>
#include <fstream>
#include <Main.h>
#include "Board.h"
#include "I2C.h"
#include "pi-blaster.h"

extern "C" void bcm_host_init( void );
extern "C" void bcm_host_deinit( void );

uint64_t Board::mTicksBase = 0;
decltype(Board::mRegisters) Board::mRegisters = decltype(Board::mRegisters)();

VCHI_INSTANCE_T Board::global_initialise_instance = nullptr;
VCHI_CONNECTION_T* Board::global_connection = nullptr;


Board::Board( Main* main )
{
	bcm_host_init();
// 	VCOSInit();

	wiringPiSetupGpio();
	PiBlasterInit( 100 );

	system( "mount -o remount,rw /data" );

	atexit( &Board::AtExit );

	std::ifstream file( "/data/flight_regs" );
	std::string line;
	if ( file.is_open() ) {
		while ( std::getline( file, line, '\n' ) ) {
			std::string key = line.substr( 0, line.find( "=" ) );
			std::string value = line.substr( line.find( "=" ) + 1 );
			mRegisters[ key ] = value;
		}
		file.close();
	}
}


Board::~Board()
{
}


void Board::AtExit()
{
}


void Board::InformLoading( int force_led )
{
	static uint64_t ticks = 0;
	static bool led_state = true;
	char cmd[256] = "";

	if ( force_led == 0 or force_led == 1 or GetTicks() - ticks >= 1000 * 250 ) {
		ticks = GetTicks();
		led_state = !led_state;
		if ( force_led == 0 or force_led == 1 ) {
			led_state = force_led;
		}
// 		sprintf( cmd, "echo %d > /sys/class/leds/led1/brightness", led_state );
		sprintf( cmd, "echo %d > /sys/class/leds/led0/brightness", led_state );
		system( cmd );
	}
}


void Board::LoadingDone()
{
	InformLoading( true );
}


void Board::setLocalTimestamp( uint32_t t )
{
	time_t tt = t;
	stime( &tt );
}


Board::Date Board::localDate()
{
	Date ret;
	struct tm* tm = nullptr;
	time_t tmstmp = time( nullptr );

	tm = localtime( &tmstmp );
	ret.year = 1900 + tm->tm_year;
	ret.month = tm->tm_mon;
	ret.day = tm->tm_mday;
	ret.hour = tm->tm_hour;
	ret.minute = tm->tm_min;
	ret.second = tm->tm_sec;

	return ret;
}


const std::string Board::LoadRegister( const std::string& name )
{
	if ( mRegisters.find( name ) != mRegisters.end() ) {
		return mRegisters[ name ];
	}
	return "";
}


int Board::SaveRegister( const std::string& name, const std::string& value )
{
	mRegisters[ name ] = value;

	std::ofstream file( "/data/flight_regs" );
	if ( file.is_open() ) {
		for ( auto reg : mRegisters ) {
			std::string line = reg.first + "=" + reg.second + "\n";
			file.write( line.c_str(), line.length() );
		}
		file.flush();
		file.close();
		sync();
		return 0;
	}

	return -1;
}


uint64_t Board::GetTicks()
{
	if ( mTicksBase == 0 ) {
		struct timespec now;
		clock_gettime( CLOCK_MONOTONIC, &now );
		mTicksBase = (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
	}

	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL - mTicksBase;
}


uint64_t Board::WaitTick( uint64_t ticks_p_second, uint64_t lastTick, uint64_t sleep_bias )
{
	uint64_t ticks = GetTicks();

	if ( ( ticks - lastTick ) < ticks_p_second ) {
		int64_t t = (int64_t)ticks_p_second - ( (int64_t)ticks - (int64_t)lastTick ) + sleep_bias;
		if ( t < 0 ) {
			t = 0;
		}
		usleep( t );
	}

	return GetTicks();
}


void Board::VCOSInit()
{
	VCHIQ_INSTANCE_T vchiq_instance;
	int success = -1;
	char response[ 128 ];

	vcos_init();

	if ( vchiq_initialise( &vchiq_instance ) != VCHIQ_SUCCESS ) {
		gDebug() << "* Failed to open vchiq instance\n";
		exit(-1);
	}

	gDebug() << "vchi_initialise\n";
	success = vchi_initialise( &global_initialise_instance );
	vcos_assert(success == 0);
	vchiq_instance = (VCHIQ_INSTANCE_T)global_initialise_instance;

	global_connection = vchi_create_connection( single_get_func_table(), vchi_mphi_message_driver_func_table() );

	gDebug() << "vchi_connect\n";
	vchi_connect( &global_connection, 1, global_initialise_instance );

	vc_vchi_gencmd_init( global_initialise_instance, &global_connection, 1 );
	vc_vchi_dispmanx_init( global_initialise_instance, &global_connection, 1 );
	vc_vchi_tv_init( global_initialise_instance, &global_connection, 1 );
	vc_vchi_cec_init( global_initialise_instance, &global_connection, 1 );

	if ( success == 0 ) {
		success = vc_gencmd( response, sizeof(response), "set_vll_dir /sd/vlls" );
		vcos_assert( success == 0 );
	}
}
