#include <unistd.h>
#include <vc_dispmanx_types.h>
#include <bcm_host.h>
#include "Board.h"

Board::Board()
	: mADC( nullptr )
{
	usleep( 1000 * 1000 * 2 );
	system( "dd if=/dev/zero of=/dev/fb0" );
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
			return (float)battery_voltage * 3.72f * 4.0f / 4096.0f;
		}
	}

	return 0.0f;
}


uint32_t Board::displayWidth()
{
	uint32_t display_width;
	uint32_t display_height;
	int ret = graphics_get_display_size( 5, &display_width, &display_height );
	return display_width;
}


uint32_t Board::displayHeight()
{
	uint32_t display_width;
	uint32_t display_height;
	int ret = graphics_get_display_size( 5, &display_width, &display_height );
	return display_height;
}
