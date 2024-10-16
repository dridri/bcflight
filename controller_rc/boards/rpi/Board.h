#pragma once

#include <byteswap.h>
#include <stdint.h>
#include <string>
#include <map>

using namespace std;


class Board
{
public:
	Board();
	~Board();

	static uint64_t GetTicks();
	static uint64_t WaitTick( uint64_t ticks_p_second, uint64_t lastTick, int64_t sleep_bias = -500 );

	static map< string, bool >& defectivePeripherals();

private:
	static uint64_t mTicksBase;
	static map< string, bool > mDefectivePeripherals;
};

uint16_t board_htons( uint16_t );
uint16_t board_ntohs( uint16_t );
uint32_t board_htonl( uint32_t );
uint32_t board_ntohl( uint32_t );
