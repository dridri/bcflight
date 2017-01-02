#ifndef BOARD_H
#define BOARD_H

#include "../../ADCs/MCP320x.h"

class Board
{
public:
	Board();
	~Board();

	float localBatteryVoltage();

private:
	MCP320x* mADC;
};

#endif // BOARD_H
