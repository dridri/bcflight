/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <execinfo.h>
#if ( BUILD_RAWWIFI == 1 )
#include "rawwifi.h"
#endif

extern "C" {
#include <interface/vmcs_host/vc_vchi_gencmd.h>
#include <interface/vmcs_host/vc_vchi_bufman.h>
#include <interface/vmcs_host/vc_tvservice.h>
#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vmcs_host/vcgencmd.h>
#include <interface/vchiq_arm/vchiq_if.h>
};

#include <wiringPi.h>
#include <fstream>
#include <Main.h>
#include "Board.h"
#include "I2C.h"
#include "PWM.h"
#include "Debug.h"

extern "C" void bcm_host_init( void );
extern "C" void bcm_host_deinit( void );
extern "C" void OMX_Init();

uint64_t Board::mTicksBase = 0;
uint64_t Board::mLastWorkJiffies = 0;
uint64_t Board::mLastTotalJiffies = 0;
bool Board::mUpdating = false;
decltype(Board::mRegisters) Board::mRegisters = decltype(Board::mRegisters)();

HookThread<Board>* Board::mStatsThread = nullptr;
uint32_t Board::mCPUTemp = 0;
uint32_t Board::mCPULoad = 0;
bool Board::mDiskFull = false;
vector< string > Board::mBoardMessages;
map< string, bool > Board::mDefectivePeripherals;

VCHI_INSTANCE_T Board::global_initialise_instance = nullptr;
VCHI_CONNECTION_T* Board::global_connection = nullptr;

#include <signal.h>

Board::Board( Main* main )
{
	bcm_host_init();
#ifdef CAMERA
	OMX_Init();
#endif
	vc_gencmd_init();
// 	VCOSInit();

	wiringPiSetup();
	wiringPiSetupGpio();

	system( "mount -o remount,rw /var" );
	system( "mkdir -p /var/VIDEO" );

	atexit( &Board::AtExit );

	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = &Board::SegFaultHandler;
	sigaction( 11, &sa, NULL );

	ifstream file( "/var/flight/registers" );
	string line;
	if ( file.is_open() ) {
		while ( getline( file, line, '\n' ) ) {
			string key = line.substr( 0, line.find( "=" ) );
			string value = line.substr( line.find( "=" ) + 1 );
			mRegisters[ key ] = value;
		}
		file.close();
	}

// 	gDebug() << readcmd( "iw list" ) << "\n";

	if ( mStatsThread == nullptr ) {
		mStatsThread = new HookThread<Board>( "board_stats", this, &Board::StatsThreadRun );
		mStatsThread->setFrequency( 1 );
		mStatsThread->Start();
	}
}


Board::~Board()
{
}


void Board::AtExit()
{
}


void Board::SegFaultHandler( int sig )
{
	string msg;
	char name[64] = "";
	Thread* thread = Thread::currentThread();

	if ( thread ) {
		thread->Stop();
		msg = "Thread \"" + thread->name() + "\" crashed !";
	} else {
		pthread_getname_np( pthread_self(), name, sizeof(name) );
		if ( name[0] == '\0' or !strcmp( name, "flight" ) ) {
			msg = "Thread <unknown> crashed !";
		} else {
			msg = "Thread \"" + string(name) + "\" crashed !";
		}
	}

	mBoardMessages.emplace_back( msg );
	gDebug() << "\e[0;41m" << msg << "\e[0m\n";

	void* array[16];
	size_t size;
	size = backtrace( array, 16 );
	fprintf( stderr, "Error: signal %d :\n", sig );
	char** trace = backtrace_symbols( array, size );
	gDebug() << "\e[91mStack trace :\e[0m\n";
	for ( size_t j = 0; j < size; j++ ) {
		gDebug() << "\e[91m" << trace[j] << "\e[0m\n";
	}

	if ( thread and thread->name() == "stabilizer" ) {
		PWM::terminate(0);
	}

	// Try to restart the thread, but not too many times
	static uint32_t total_restarts = 0;
	if ( thread and total_restarts < 4 ) {
		total_restarts++;
		thread->Recover();
	}

	while ( 1 ) {
		usleep( 1000 * 1000 * 10 );
	}
}


vector< string > Board::messages()
{
	vector< string > msgs;

	for ( auto defect : mDefectivePeripherals ) {
		if ( defect.second == true ) {
			msgs.emplace_back( defect.first + " is defective !" );
		}
	}
	if ( mDiskFull ) {
		msgs.emplace_back( "No free space on disk" );
	}
	if ( mCPUTemp > 80 ) {
		msgs.emplace_back( "High CPU Temp (" + to_string(mCPUTemp) + "\xB0"" C)" );
	}

	msgs.insert( msgs.end(), mBoardMessages.begin(), mBoardMessages.end() );
	return msgs;
}


map< string, bool >& Board::defectivePeripherals()
{
	return mDefectivePeripherals;
}


void Board::UpdateFirmwareData( const uint8_t* buf, uint32_t offset, uint32_t size )
{
	gDebug() << "Saving Flight Controller firmware update (" << size << " bytes at offset " << offset << ")\n";

	if ( offset == 0 ) {
		system( "rm -f /tmp/flight_update" );
		system( "touch /tmp/flight_update" );
	}

	fstream firmware( "/tmp/flight_update", ios_base::in | ios_base::out | ios_base::binary );
	firmware.seekg( offset, firmware.beg );
	firmware.write( (char*)buf, size );
	firmware.close();

}


static uint32_t crc32( const uint8_t* buf, uint32_t len )
{
	uint32_t k = 0;
	uint32_t crc = 0;

	crc = ~crc;

	while ( len-- ) {
		crc ^= *buf++;
		for ( k = 0; k < 8; k++ ) {
			crc = ( crc & 1 ) ? ( (crc >> 1) ^ 0x82f63b78 ) : ( crc >> 1 );
		}
	}

	return ~crc;
}


void Board::UpdateFirmwareProcess( uint32_t crc )
{
	if ( mUpdating ) {
		return;
	}
	gDebug() << "Updating Flight Controller firmware\n";
	mUpdating = true;

	ifstream firmware( "/tmp/flight_update" );
	if ( firmware.is_open() ) {
		firmware.seekg( 0, firmware.end );
		int length = firmware.tellg();
		uint8_t* buf = new uint8_t[ length + 1 ];
		firmware.seekg( 0, firmware.beg );
		firmware.read( (char*)buf, length );
		buf[length] = 0;
		firmware.close();
		if ( crc32( buf, length ) != crc ) {
			gDebug() << "ERROR : Wrong CRC32 for firmware upload, please retry\n";
			mUpdating = false;
			return;
		}
		
		system( "rm -f /tmp/update.sh" );
	
		ofstream file( "/tmp/update.sh" );
		file << "#!/bin/bash\n\n";
		file << "systemctl stop flight.service &\n";
		file << "sleep 1\n";
		file << "systemctl stop flight.service &\n";
		file << "sleep 1\n";
		file << "killall -9 flight\n";
		file << "sleep 0.5\n";
		file << "killall -9 flight\n";
		file << "sleep 0.5\n";
		file << "killall -9 flight\n";
		file << "sleep 1\n";
		file << "rm /var/flight/flight\n";
		file << "cp /tmp/flight_update /var/flight/flight\n";
		file << "sleep 2\n";
		file << "rm /tmp/flight_update\n";
		file << "sleep 1\n";
		file << "chmod +x /var/flight/flight\n";
		file << "sleep 1\n";
		file << "systemctl start flight.service\n";
		file.close();

		system( "nohup sh /tmp/update.sh &" );
	}
}


void Board::Reset()
{
	gDebug() << "Restarting Flight Controller service\n";
	system( "rm -f /tmp/reset.sh" );

	ofstream file( "/tmp/reset.sh" );
	file << "#!/bin/bash\n\n";
	file << "systemctl stop flight.service &\n";
	file << "sleep 1\n";
	file << "systemctl stop flight.service &\n";
	file << "sleep 1\n";
	file << "killall -9 flight\n";
	file << "sleep 0.5\n";
	file << "killall -9 flight\n";
	file << "sleep 0.5\n";
	file << "killall -9 flight\n";
	file << "sleep 1\n";
	file << "systemctl start flight.service\n";
	file.close();

	system( "sh /tmp/reset.sh" );
}


static string readproc( const string& filename, const string& entry = "", const string& delim = ":" )
{
	char buf[1024] = "";
	string res = "";
	ifstream file( filename );

	if ( file.is_open() ) {
		if ( entry.length() == 0 or entry == "" ) {
			file.readsome( buf, sizeof( buf ) );
			res = buf;
		} else {
			while ( file.good() ) {
				file.getline( buf, sizeof(buf), '\n' );
				if ( strstr( buf, entry.c_str() ) ) {
					char* s = strstr( buf, delim.c_str() ) + delim.length();
					while ( *s == ' ' or *s == '\t' ) {
						s++;
					}
					char* end = s;
					while ( *end != '\n' and *end++ );
					*end = 0;
					res = string( s );
					break;
				}
			}
		}
		file.close();
	}

	return res;
}


string Board::readcmd( const string& cmd, const string& entry, const string& delim )
{
	char buf[1024] = "";
	string res = "";
	FILE* fp = popen( cmd.c_str(), "r" );
	if ( !fp ) {
		printf( "popen failed : %s\n", strerror( errno ) );
		return "";
	}

	if ( entry.length() == 0 or entry == "" ) {
		while ( fgets( buf, sizeof(buf), fp ) ) {
			res += buf;
		}
	} else {
		while ( fgets( buf, sizeof(buf), fp ) ) {
			if ( strstr( buf, entry.c_str() ) ) {
				char* s = strstr( buf, delim.c_str() ) + delim.length();
				while ( *s == ' ' or *s == '\t' ) {
					s++;
				}
				char* end = s;
				while ( *end != '\n' and *end++ );
				*end = 0;
				res = string( s );
				break;
			}
		}
	}

	pclose( fp );
	return res;
}


string Board::infos()
{
	string res = "";

	res += "Type:" BOARD "\n";
	res += "Firmware version:" VERSION_STRING "\n";
	res += "Model:" + readproc( "/proc/cpuinfo", "Hardware" ) + string( "\n" );
	res += "CPU:" + readproc( "/proc/cpuinfo", "model name" ) + string( "\n" );
	res += "CPU cores count:" + to_string( sysconf( _SC_NPROCESSORS_ONLN ) ) + string( "\n" );
	res += "CPU frequency:" + to_string( atoi( readproc( "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq" ).c_str() ) / 1000 ) + string( "MHz\n" );
	res += "GPU frequency:" + to_string( atoi( readcmd( "vcgencmd measure_clock core", "frequency", "=" ).c_str() ) / 1000000 ) + string( "MHz\n" );
	res += "SoC voltage:" + readcmd( "vcgencmd measure_volts core", "volt", "=" ) + string( "\n" );
	res += "RAM:" + readcmd( "vcgencmd get_mem arm", "arm", "=" ) + string( "\n" );
	res += "VRAM:" + readcmd( "vcgencmd get_mem gpu", "gpu", "=" ) + string( "\n" );

	return res;
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
		sprintf( cmd, "echo %d > /sys/class/leds/led0/brightness", led_state );
		system( cmd );
	}
}


void Board::LoadingDone()
{
	InformLoading( 1 );
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


const uint32_t Board::LoadRegisterU32( const string& name, uint32_t def )
{
	string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return strtoul( str.c_str(), nullptr, 10 );
	}
	return def;
}


const float Board::LoadRegisterFloat( const string& name, float def )
{
	string str = LoadRegister( name );
	if ( str != "" and str.length() > 0 ) {
		return atof( str.c_str() );
	}
	return def;
}


const string Board::LoadRegister( const string& name )
{
	if ( mRegisters.find( name ) != mRegisters.end() ) {
		return mRegisters[ name ];
	}
	return "";
}


int Board::SaveRegister( const string& name, const string& value )
{
	mRegisters[ name ] = value;

	ofstream file( "/var/flight/registers" );
	if ( file.is_open() ) {
		for ( auto reg : mRegisters ) {
			string line = reg.first + "=" + reg.second + "\n";
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


bool Board::StatsThreadRun()
{
	char cpu_temp[256];
	vc_gencmd( cpu_temp, sizeof(cpu_temp)-1, "measure_temp" );
	mCPUTemp = atoi( strchr( cpu_temp, '=' ) + 1 );

	uint32_t jiffies[7];
	stringstream ss;
	ss.str( readcmd( "cat /proc/stat | grep \"cpu \" | cut -d' ' -f2-", "", "" ) ); 

	ss >> jiffies[0];
	ss >> jiffies[1];
	ss >> jiffies[2];
	ss >> jiffies[3];
	ss >> jiffies[4];
	ss >> jiffies[5];
	ss >> jiffies[6];

	uint64_t work_jiffies = jiffies[0] + jiffies[1] + jiffies[2];
	uint64_t total_jiffies = jiffies[0] + jiffies[1] + jiffies[2] + jiffies[3] + jiffies[4] + jiffies[5] + jiffies[6];

	mCPULoad = ( work_jiffies - mLastWorkJiffies ) * 100 / ( total_jiffies - mLastTotalJiffies );

	mLastWorkJiffies = work_jiffies;
	mLastTotalJiffies = total_jiffies;

	return true;
}


uint32_t Board::CPULoad()
{
	return mCPULoad;
}


uint32_t Board::CPUTemp()
{
	return mCPUTemp;
}


uint32_t Board::FreeDiskSpace()
{
	return strtoul( readproc( "df -P /data | tail -n1 | sed 's/  */ /g' | cut -d' ' -f4" ).c_str(), nullptr, 0 );
}


void Board::setDiskFull()
{
	mDiskFull = true;
}


static int tun_alloc( const char* dev )
{
	struct ifreq ifr;
	int fd, err;

	if ( ( fd = open( "/dev/net/tun", O_RDWR ) ) < 0 ) {
		printf( "open error : %s\n", strerror( errno ) );
		return fd;
	}

	memset( &ifr, 0, sizeof(ifr) );
	ifr.ifr_flags = 0x0001 | 0x1000;
// 	ifr.ifr_flags = 0x0002;
	strncpy( ifr.ifr_name, dev, IFNAMSIZ );
	if ( ( err = ioctl( fd, _IOW('T', 202, int), (void *) &ifr ) ) < 0 ) {
		printf( "ioctl error : %s\n", strerror( errno ) );
		close( fd );
		return err;
	}

	return fd;
}


static void cmd( const char* fmt, ... )
{
	char buffer[256];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buffer, 256, fmt, args );
	perror( buffer );
	va_end( args );
	system( buffer );
}


#if ( BUILD_RAWWIFI == 1 )
typedef struct tun_args {
	int fd;
	rawwifi_t* rwifi;
} tun_args;


static void* thread_rx( void* argp )
{
	tun_args* args = (tun_args*)argp;
	uint8_t buffer[16384] = {0};

	while ( 1 ) {
		uint32_t valid = 0;
		int nread = rawwifi_recv( args->rwifi, buffer, sizeof(buffer), &valid );
		if ( nread > 0 ) {
			write( args->fd, buffer, nread );
		}
	}

	return NULL;
}

static void* thread_tx( void* argp )
{
	tun_args* args = (tun_args*)argp;
	uint8_t buffer[16384] = {0};

	while ( 1 ) {
		int nread = read( args->fd, buffer, sizeof(buffer) );
		if ( nread < 0 ) {
			gDebug() << "Error reading from air0 interface\n";
			close( args->fd );
			break;
		}
		rawwifi_send_retry( args->rwifi, buffer, nread, 1 );
	}

	return NULL;
}
#endif


void Board::EnableTunDevice()
{
	if ( true ) { // TODO : detect if rawwifi is currently in use
#if ( BUILD_RAWWIFI == 1 )
		int port = 128; // TODO : Dynamically find unused ports

		int fd = tun_alloc( "air0" );
	// 	fcntl( fd, F_SETFL, O_NONBLOCK );
		gDebug() << "tunnel fd ready\n";

		rawwifi_t* rwifi = rawwifi_init( "wlan0", port, port + 1, 1, -1 ); // TODO : Use same device as RawWifi
		gDebug() << "tunnel rawwifi ready\n";

		// TODO : use ThreadHooks
		tun_args* args = (tun_args*)malloc(sizeof(tun_args));
		args->fd = fd;
		args->rwifi = rwifi;
		pthread_t thid1, thid2;
		pthread_create( &thid1, nullptr, thread_rx, (void*)args );
		pthread_create( &thid2, nullptr, thread_tx, (void*)args );

		// TODO : use libnl
		usleep( 1000 * 1000 );
		cmd( "ip link set air0 up" );
		usleep( 1000 * 500 );
		cmd( "ip addr add 10.0.0.1/24 dev air0" );
#endif
		cmd( "systemctl start sshd.service" );
	} else {
		// TODO
	}
}


void Board::DisableTunDevice()
{
	// TODO
}


extern "C" uint32_t _mem_usage()
{
	return 0; // TODO
}


uint16_t board_htons( uint16_t v )
{
	return htons( v );
}


uint16_t board_ntohs( uint16_t v )
{
	return ntohs( v );
}


uint32_t board_htonl( uint32_t v )
{
	return htonl( v );
}


uint32_t board_ntohl( uint32_t v )
{
	return ntohl( v );
}
