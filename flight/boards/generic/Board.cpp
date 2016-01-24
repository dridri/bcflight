#include <time.h>
#include <sys/time.h>
#include <Main.h>
#include "Board.h"
#include "I2C.h"


uint64_t Board::mTicksBase = 0;
decltype(Board::mRegisters) Board::mRegisters = decltype(Board::mRegisters)();


Board::Board( Main* main )
{
	srand( time( nullptr ) );
	// hardware-specific initialization should be done here or directly in startup.cpp
	// this->mRegisters should be loaded here
}


Board::~Board()
{
}


void Board::InformLoading( int force_led )
{
	// Controls loading LED.
	// if force_led != -1, then this function should turn on (1) or off (0) the led according to this variablezz
	// Otherwise, make it blink
}


void Board::LoadingDone()
{
	InformLoading( true );
}


void Board::setLocalTimestamp( uint32_t t )
{
}


Board::Date Board::localDate()
{
	Date ret;
	memset( &ret, 0, sizeof(ret) );
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

	// Save register(s) here, and return 0 on success or -1 on error

	return 0;
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
