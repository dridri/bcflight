#include <time.h>
#include <sys/time.h>
#include <Main.h>
#include "Board.h"
#include "I2C.h"


uint64_t Board::mTicksBase = 0;


Board::Board( Main* main )
{
	srand( time( nullptr ) );
}


Board::~Board()
{
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
