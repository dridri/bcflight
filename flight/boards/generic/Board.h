#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <list>

class Main;

class Board
{
public:
	Board( Main* main );
	~Board();

	static uint64_t GetTicks();

private:
	static uint64_t mTicksBase;
};

#endif // BOARD_H
