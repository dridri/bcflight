#include "Board.h"

Board::Board()
	: mADC( nullptr )
{
}


Board::~Board()
{
}


float Board::localBatteryVoltage()
{
	if ( mADC == nullptr ) {
		mADC = new MCP320x( "/dev/spidev0.0" );
	}

	if ( mADC ) {
		uint16_t battery_voltage = mADC->Read( 0 );
		if ( battery_voltage != 0 ) {
			return (float)battery_voltage * 4.80f * 4.0f / 4096.0f;
		}
	}

	return 0.0f;
}
