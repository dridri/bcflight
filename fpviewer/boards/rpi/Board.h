#ifndef BOARD_H
#define BOARD_H

#include "../../ADCs/MCP320x.h"

class Board
{
public:
	Board();
	~Board();

	uint32_t displayWidth();
	uint32_t displayHeight();
	float localBatteryVoltage();

private:
	MCP320x* mADC;
};

#endif // BOARD_H
