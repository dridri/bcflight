#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include "Board.h"


uint64_t Board::mTicksBase = 0;
map< string, bool > Board::mDefectivePeripherals;


Board::Board()
{
}


Board::~Board()
{
}


map< string, bool >& Board::defectivePeripherals()
{
	return mDefectivePeripherals;
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


uint64_t Board::WaitTick( uint64_t ticks_p_second, uint64_t lastTick, int64_t sleep_bias )
{
	// fDebug( ticks_p_second, lastTick, sleep_bias );
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
