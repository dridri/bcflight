#ifndef MCP320X_H
#define MCP320X_H

#include <stdint.h>
#include <linux/spi/spidev.h>

class MCP320x
{
public:
	MCP320x();
	~MCP320x();

	uint16_t Read( uint8_t channel );

private:
	int mFD;
	struct spi_ioc_transfer mXFer[10];
};

#endif // MCP320X_H
